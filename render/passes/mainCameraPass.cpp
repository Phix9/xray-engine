#include "render/passes/mainCameraPass.hpp"
#include <mesh_vert.hpp>
#include <mesh_frag.hpp>

namespace XrayEngine
{ 
    void MainCameraPass::Initialize(const RenderPassInitInfo *initInfo)
    {
        RenderPass::Initialize(initInfo);

        SetupDescriptorSetLayout();
        SetupRenderPass();
        SetupPipelines();
        SetupSwapchainFramebuffers();
        SetupDescriptorSet();        
    }

    void MainCameraPass::Quit()
    {
        vulkanRHI->DestroyRenderPass(renderPass);
        
        for(const vk::DescriptorSetLayout &layout : descriptorSetLayouts)
        {
            vulkanRHI->DestroyDescriptorSetLayout(layout);
        }

        for (const RenderPipeline &renderPipeline : renderPipelines)
        {
            vulkanRHI->DestroyPipelineLayout(renderPipeline.layout);
            vulkanRHI->DestroyPipeline(renderPipeline.pipeline);
        }

        for (const vk::Framebuffer &framebuffer : swapchainFramebuffers)
        {
            vulkanRHI->DestroyFramebuffer(framebuffer);
        }
        
        vulkanRHI->DestroySampler(sampler);
    }

    void MainCameraPass::Draw()
    {
        uint32_t perFrameDynamicOffset = RoundUp(renderResource->storage
            .ringBufferEnds[vulkanRHI->GetCurrentFrameIndex()],
            renderResource->storage.minStorageBufferOffsetAlignment);

        renderResource->storage.ringBufferEnds[vulkanRHI->GetCurrentFrameIndex()] =
            perFrameDynamicOffset + sizeof(MeshPerFrameStorageBufferObject);

        (*reinterpret_cast<MeshPerFrameStorageBufferObject*>(
            reinterpret_cast<uintptr_t>(
                renderResource-> storage.mappedMemory) +
            perFrameDynamicOffset)) = renderResource->meshPerFrameStorageBufferObject;

        vk::Rect2D renderArea;
        renderArea.setOffset({0, 0})
            .setExtent(vulkanRHI->GetSwapchainInfo().extent);

        std::vector<vk::ClearValue> clearValues(eMainCameraPassAttachmentCount);
        clearValues[eMainCameraPassAttachmentSwapchainImage].setColor({0.0f, 0.0f, 0.0f, 0.0f});
        clearValues[eMainCameraPassAttachmentDepth].setDepthStencil({1.0f, 0});

        vk::RenderPassBeginInfo beginInfo;
        beginInfo.setRenderPass(renderPass)
            .setFramebuffer(swapchainFramebuffers[vulkanRHI->GetCurrentSwapchainImageIndex()])
            .setRenderArea(renderArea)
            .setClearValues(clearValues);

        vulkanRHI->CommandBeginRenderPass(vulkanRHI->GetCurrentCommandBuffer(), beginInfo, vk::SubpassContents::eInline);

        vulkanRHI->CommandBindDescriptorSet(vulkanRHI->GetCurrentCommandBuffer(), vk::PipelineBindPoint::eGraphics, renderPipelines[eRenderPipelineTypeMesh].layout, 0, {descriptorSets}, {perFrameDynamicOffset});

        vulkanRHI->CommandBindPipeline(vulkanRHI->GetCurrentCommandBuffer(), vk::PipelineBindPoint::eGraphics, renderPipelines[eRenderPipelineTypeMesh].pipeline);

        vulkanRHI->CommandSetViewport(vulkanRHI->GetCurrentCommandBuffer(), 0, {vulkanRHI->GetSwapchainInfo().viewport});

        vulkanRHI->CommandSetScissor(vulkanRHI->GetCurrentCommandBuffer(), 0, {vulkanRHI->GetSwapchainInfo().scissor});

        vulkanRHI->CommandBindVertexBuffers(vulkanRHI->GetCurrentCommandBuffer(), 0, {renderResource->mesh.vertexBuffer}, {0});

        vulkanRHI->CommandBindIndexBuffer(vulkanRHI->GetCurrentCommandBuffer(), renderResource->mesh.indexBuffer, 0, vk::IndexType::eUint32);

        vulkanRHI->CommandDrawIndexed(vulkanRHI->GetCurrentCommandBuffer(), renderResource->mesh.indexCount, 1, 0, 0, 0);

        vulkanRHI->CommandEndRenderPass(vulkanRHI->GetCurrentCommandBuffer());
    }

    void MainCameraPass::SetupRenderPass()
    {
        // Attachment Description
        std::vector<vk::AttachmentDescription> attachmentDescriptions(eMainCameraPassAttachmentCount);

        vk::AttachmentDescription &swapchainImageAttachmentDescription = attachmentDescriptions[eMainCameraPassAttachmentSwapchainImage];
        swapchainImageAttachmentDescription.setFormat(vulkanRHI->GetSwapchainInfo().format)
            .setSamples(vk::SampleCountFlagBits::e1)
            .setInitialLayout(vk::ImageLayout::eUndefined)
            .setFinalLayout(vk::ImageLayout::ePresentSrcKHR)
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
            .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);

        vk::AttachmentDescription &depthAttachmentDescription = attachmentDescriptions[eMainCameraPassAttachmentDepth];
        depthAttachmentDescription.setFormat(vulkanRHI->GetDepthImageInfo().format)
            .setSamples(vk::SampleCountFlagBits::e1)
            .setInitialLayout(vk::ImageLayout::eUndefined)
            .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
            .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);

        // Subpass
        std::vector<vk::SubpassDescription> subpasses(eMainCameraSubpassCount);

        vk::AttachmentReference basePassColorAttachmentReference;
        basePassColorAttachmentReference.setAttachment(eMainCameraPassAttachmentSwapchainImage)
            .setLayout(vk::ImageLayout::eColorAttachmentOptimal);
        vk::AttachmentReference basePassDepthAttachmentReference;
        basePassDepthAttachmentReference.setAttachment(eMainCameraPassAttachmentDepth)
            .setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

        vk::SubpassDescription &basePass = subpasses[eMainCameraSubpassBasePass];
        basePass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
            .setColorAttachments(basePassColorAttachmentReference)
            .setPDepthStencilAttachment(&basePassDepthAttachmentReference);

        // Dependencies
        std::vector<vk::SubpassDependency> dependencies(1);

        vk::SubpassDependency &basePassDependency = dependencies[0];
        basePassDependency.setSrcSubpass(VK_SUBPASS_EXTERNAL)
            .setDstSubpass(eMainCameraSubpassBasePass)
            .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eLateFragmentTests)
            .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests)
            .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite)
            .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite);

        // RenderPass
        vk::RenderPassCreateInfo createInfo;
        createInfo.setAttachments(attachmentDescriptions)
            .setSubpasses(subpasses)
            .setDependencies(dependencies);

        renderPass = vulkanRHI->CreateRenderPass(createInfo);
    }

    void MainCameraPass::SetupPipelines()
    {
        renderPipelines.resize(eRenderPipelineTypeCount);

        vk::ShaderModule vertShader = vulkanRHI->CreateShaderModule(MESH_VERT);
        vk::ShaderModule fragShader = vulkanRHI->CreateShaderModule(MESH_FRAG);

        // layout (default)
        vk::PipelineLayoutCreateInfo layoutCreateInfo;
        layoutCreateInfo.setSetLayouts(descriptorSetLayouts);
        renderPipelines[eRenderPipelineTypeMesh].layout = vulkanRHI->CreatePipelineLayout(layoutCreateInfo);

        // 1. shader
        vk::PipelineShaderStageCreateInfo vertShaderStageCreateInfo;
        vertShaderStageCreateInfo.setStage(vk::ShaderStageFlagBits::eVertex)
            .setModule(vertShader)
            .setPName("main");
        vk::PipelineShaderStageCreateInfo fragShaderStageCreateIngo;
        fragShaderStageCreateIngo.setStage(vk::ShaderStageFlagBits::eFragment)
            .setModule(fragShader)
            .setPName("main");
        std::vector<vk::PipelineShaderStageCreateInfo> shaderStages{vertShaderStageCreateInfo, fragShaderStageCreateIngo};

        // 2. vertex input
        vk::PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo;
        std::vector<vk::VertexInputAttributeDescription> attributeDescription = MeshVertex::GetAttributeDescriptions();
        std::vector<vk::VertexInputBindingDescription> bindingDescriptions = MeshVertex::GetBindingDescriptions();
        vertexInputStateCreateInfo.setVertexAttributeDescriptions(attributeDescription)
            .setVertexBindingDescriptions(bindingDescriptions);

        // 3. input assembly
        vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo;
        inputAssemblyStateCreateInfo.setPrimitiveRestartEnable(false)
            .setTopology(vk::PrimitiveTopology::eTriangleList);

        // 4. viewport
        vk::PipelineViewportStateCreateInfo viewportStateCreateInfo;
        vk::Viewport viewport = vulkanRHI->GetSwapchainInfo().viewport;
        vk::Rect2D scissor = vulkanRHI->GetSwapchainInfo().scissor;
        viewportStateCreateInfo.setViewports(viewport)
            .setScissors(scissor);

        // 5. rasterization
        vk::PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo;
        rasterizationStateCreateInfo.setRasterizerDiscardEnable(false)
            .setCullMode(vk::CullModeFlagBits::eBack)
            .setFrontFace(vk::FrontFace::eCounterClockwise)
            .setPolygonMode(vk::PolygonMode::eFill)
            .setLineWidth(1.0f);

        // 6. multisample
        vk::PipelineMultisampleStateCreateInfo multisampleStateCreateInfo;
        multisampleStateCreateInfo.setSampleShadingEnable(false)
            .setRasterizationSamples(vk::SampleCountFlagBits::e1);

        // 7. color blending
        vk::PipelineColorBlendAttachmentState colorBlendAttachmentState;
        colorBlendAttachmentState.setBlendEnable(false)
            .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                               vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);

        vk::PipelineColorBlendStateCreateInfo colorBlendCreateInfo;
        colorBlendCreateInfo.setLogicOpEnable(false)
            .setAttachments(colorBlendAttachmentState);

        // 8. depth and stencil test
        vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo;
        depthStencilStateCreateInfo.setDepthTestEnable(true)
            .setDepthWriteEnable(true)
            .setDepthCompareOp(vk::CompareOp::eLess)
            .setDepthBoundsTestEnable(false)
            .setStencilTestEnable(false);

        // 9. dynamic state: viewport/scissor are re-specified every frame so that a
        // window resize does not require rebuilding the pipeline.
        std::vector<vk::DynamicState> dynamicStates{vk::DynamicState::eViewport, vk::DynamicState::eScissor};
        vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo;
        dynamicStateCreateInfo.setDynamicStates(dynamicStates);

        vk::GraphicsPipelineCreateInfo createInfo;
        createInfo.setPVertexInputState(&vertexInputStateCreateInfo)
            .setPInputAssemblyState(&inputAssemblyStateCreateInfo)
            .setStages(shaderStages)
            .setPViewportState(&viewportStateCreateInfo)
            .setPRasterizationState(&rasterizationStateCreateInfo)
            .setPMultisampleState(&multisampleStateCreateInfo)
            .setPColorBlendState(&colorBlendCreateInfo)
            .setPDepthStencilState(&depthStencilStateCreateInfo)
            .setPDynamicState(&dynamicStateCreateInfo)
            .setRenderPass(renderPass)
            .setSubpass(eMainCameraSubpassBasePass)
            .setLayout(renderPipelines[eRenderPipelineTypeMesh].layout);

        renderPipelines[eRenderPipelineTypeMesh].pipeline = vulkanRHI->CreateGraphicsPipeline(createInfo);

        vulkanRHI->DestroyShaderModule(vertShader);
        vulkanRHI->DestroyShaderModule(fragShader);
    }

    void MainCameraPass::SetupSwapchainFramebuffers()
    {
        swapchainFramebuffers.resize(vulkanRHI->GetSwapchainImageViews().size());

        for (int i = 0; i < swapchainFramebuffers.size(); i++)
        {
            std::vector<vk::ImageView> frameBufferAttachments{vulkanRHI->GetSwapchainImageViews()[i],
                                                              vulkanRHI->GetDepthImageInfo().imageView};

            vk::FramebufferCreateInfo createInfo;
            createInfo.setAttachments(frameBufferAttachments)
                .setWidth(vulkanRHI->GetSwapchainInfo().extent.width)
                .setHeight(vulkanRHI->GetSwapchainInfo().extent.height)
                .setRenderPass(renderPass)
                .setLayers(1);

            swapchainFramebuffers[i] = vulkanRHI->CreateFramebuffer(createInfo);
        }
    }

    void MainCameraPass::SetupDescriptorSetLayout()
    {
        descriptorSetLayouts.resize(eDescriptorSetLayoutCount);

        {
            std::vector<vk::DescriptorSetLayoutBinding> perFrameBindings;
            perFrameBindings.resize(1);

            perFrameBindings[0].setBinding(0)
                .setDescriptorType(vk::DescriptorType::eStorageBufferDynamic)
                .setDescriptorCount(1)
                .setStageFlags(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
         
            vk::DescriptorSetLayoutCreateInfo createInfo;
            createInfo.setBindings(perFrameBindings);

            descriptorSetLayouts[eDescriptorSetLayoutPerFrame] = vulkanRHI->CreateDescriptorSetLayout(createInfo);
        }

        {
            std::vector<vk::DescriptorSetLayoutBinding> meshPerMaterialBindings;
            meshPerMaterialBindings.resize(5);

            meshPerMaterialBindings[0].setBinding(0)
                .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                .setDescriptorCount(1)
                .setStageFlags(vk::ShaderStageFlagBits::eFragment);

            meshPerMaterialBindings[1].setBinding(1)
                .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                .setDescriptorCount(1)
                .setStageFlags(vk::ShaderStageFlagBits::eFragment);
            
            meshPerMaterialBindings[2].setBinding(2)
                .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                .setDescriptorCount(1)
                .setStageFlags(vk::ShaderStageFlagBits::eFragment);
            
            meshPerMaterialBindings[3].setBinding(3)
                .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                .setDescriptorCount(1)
                .setStageFlags(vk::ShaderStageFlagBits::eFragment);
            
            meshPerMaterialBindings[4].setBinding(4)
                .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                .setDescriptorCount(1)
                .setStageFlags(vk::ShaderStageFlagBits::eFragment);
            vk::DescriptorSetLayoutCreateInfo createInfo;
            createInfo.setBindings(meshPerMaterialBindings);

            descriptorSetLayouts[eDescriptorSetLayoutMeshPerMaterial] = vulkanRHI->CreateDescriptorSetLayout(createInfo);
        }
    }

    void MainCameraPass::SetupDescriptorSet()
    {
        descriptorSets.resize(eDescriptorSetLayoutCount);

        {
            vk::DescriptorSetAllocateInfo allocateInfo;
            allocateInfo.setDescriptorPool(vulkanRHI->GetDescriptorPool())
                .setDescriptorSetCount(1)
                .setSetLayouts(descriptorSetLayouts[eDescriptorSetLayoutPerFrame]);

            descriptorSets[eDescriptorSetLayoutPerFrame] = vulkanRHI->AllocateDescriptorSets(allocateInfo);

            vk::DescriptorBufferInfo bufferInfo;
            bufferInfo.setOffset(0)
                .setRange(sizeof(MeshPerFrameStorageBufferObject))
                .setBuffer(renderResource->storage.ringBuffer);

            std::vector<vk::WriteDescriptorSet> writes;
            writes.resize(1);

            writes[0].setDescriptorType(vk::DescriptorType::eStorageBufferDynamic)
                .setBufferInfo(bufferInfo)
                .setDstSet(descriptorSets[eDescriptorSetLayoutPerFrame])
                .setDstBinding(0)
                .setDescriptorCount(1);

            vulkanRHI->UpdateDescriptorSets(writes, {});
        }

        {
            vk::DescriptorSetAllocateInfo allocateInfo;
            allocateInfo.setDescriptorPool(vulkanRHI->GetDescriptorPool())
                .setDescriptorSetCount(1)
                .setSetLayouts(descriptorSetLayouts[eDescriptorSetLayoutMeshPerMaterial]);

            descriptorSets[eDescriptorSetLayoutMeshPerMaterial] = vulkanRHI->AllocateDescriptorSets(allocateInfo);

            sampler = vulkanRHI->CreateDefaultLinearSampler();

            vk::DescriptorImageInfo baseColorImageInfo;
            baseColorImageInfo.setSampler(sampler)
                .setImageView(renderResource->material.baseColor.imageView)
                .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
            vk::DescriptorImageInfo metallicImageInfo;
            metallicImageInfo.setSampler(sampler)
                .setImageView(renderResource->material.metallic.imageView)
                .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
            vk::DescriptorImageInfo roughnessImageInfo;
            roughnessImageInfo.setSampler(sampler)
                .setImageView(renderResource->material.roughness.imageView)
                .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
            vk::DescriptorImageInfo normalImageInfo;
            normalImageInfo.setSampler(sampler)
                .setImageView(renderResource->material.normal.imageView)
                .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
            vk::DescriptorImageInfo occlusionImageInfo;
            occlusionImageInfo.setSampler(sampler)
                .setImageView(renderResource->material.occlusion.imageView)
                .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);

            std::vector<vk::WriteDescriptorSet> writes;
            writes.resize(5);

            writes[0].setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                .setImageInfo(baseColorImageInfo)
                .setDstSet(descriptorSets[eDescriptorSetLayoutMeshPerMaterial])
                .setDstBinding(0)
                .setDescriptorCount(1);
            writes[1].setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                .setImageInfo(metallicImageInfo)
                .setDstSet(descriptorSets[eDescriptorSetLayoutMeshPerMaterial])
                .setDstBinding(1)
                .setDescriptorCount(1);
            writes[2].setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                .setImageInfo(roughnessImageInfo)
                .setDstSet(descriptorSets[eDescriptorSetLayoutMeshPerMaterial])
                .setDstBinding(2)
                .setDescriptorCount(1);
            writes[3].setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                .setImageInfo(normalImageInfo)
                .setDstSet(descriptorSets[eDescriptorSetLayoutMeshPerMaterial])
                .setDstBinding(3)
                .setDescriptorCount(1);
            writes[4].setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                .setImageInfo(occlusionImageInfo)
                .setDstSet(descriptorSets[eDescriptorSetLayoutMeshPerMaterial])
                .setDstBinding(4)
                .setDescriptorCount(1);

            vulkanRHI->UpdateDescriptorSets(writes, {});
        }
    }
}