#include "render/passes/mainCameraPass.hpp"
#include <mesh_vert.hpp>
#include <mesh_frag.hpp>
#include <mesh_gbuffer_frag.hpp>
#include <deferred_lighting_vert.hpp>
#include <deferred_lighting_frag.hpp>

namespace XrayEngine
{ 
    void MainCameraPass::Initialize(const RenderPassInitInfo *initInfo)
    {
        RenderPass::Initialize(initInfo);

        SetupAttachment();
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

        std::vector<vk::DescriptorSet> meshDescriptorSets{
            descriptorSets[eDescriptorSetLayoutPerFrame],
            descriptorSets[eDescriptorSetLayoutMeshPerMaterial]};

        vulkanRHI->CommandBindDescriptorSet(vulkanRHI->GetCurrentCommandBuffer(), vk::PipelineBindPoint::eGraphics, renderPipelines[eRenderPipelineTypeMeshGBuffer].layout, 0, meshDescriptorSets, {perFrameDynamicOffset});

        vulkanRHI->CommandBindPipeline(vulkanRHI->GetCurrentCommandBuffer(), vk::PipelineBindPoint::eGraphics, renderPipelines[eRenderPipelineTypeMeshGBuffer].pipeline);

        vulkanRHI->CommandSetViewport(vulkanRHI->GetCurrentCommandBuffer(), 0, {vulkanRHI->GetSwapchainInfo().viewport});

        vulkanRHI->CommandSetScissor(vulkanRHI->GetCurrentCommandBuffer(), 0, {vulkanRHI->GetSwapchainInfo().scissor});

        vulkanRHI->CommandBindVertexBuffers(vulkanRHI->GetCurrentCommandBuffer(), 0, {renderResource->mesh.vertexBuffer}, {0});

        vulkanRHI->CommandBindIndexBuffer(vulkanRHI->GetCurrentCommandBuffer(), renderResource->mesh.indexBuffer, 0, vk::IndexType::eUint32);

        vulkanRHI->CommandDrawIndexed(vulkanRHI->GetCurrentCommandBuffer(), renderResource->mesh.indexCount, 1, 0, 0, 0);

        vulkanRHI->CommandNextSubpass(vulkanRHI->GetCurrentCommandBuffer(), vk::SubpassContents::eInline);

        std::vector<vk::DescriptorSet> deferredLightingDescriptorSets{
            descriptorSets[eDescriptorSetLayoutPerFrame],
            descriptorSets[eDescriptorSetLayoutDeferredLighting]};

        vulkanRHI->CommandBindPipeline(vulkanRHI->GetCurrentCommandBuffer(), vk::PipelineBindPoint::eGraphics, renderPipelines[eRenderPipelineTypeDeferredLighting].pipeline);

        vulkanRHI->CommandBindDescriptorSet(vulkanRHI->GetCurrentCommandBuffer(), vk::PipelineBindPoint::eGraphics, renderPipelines[eRenderPipelineTypeDeferredLighting].layout, 0, deferredLightingDescriptorSets, {perFrameDynamicOffset});

        vulkanRHI->CommandSetViewport(vulkanRHI->GetCurrentCommandBuffer(), 0, {vulkanRHI->GetSwapchainInfo().viewport});

        vulkanRHI->CommandSetScissor(vulkanRHI->GetCurrentCommandBuffer(), 0, {vulkanRHI->GetSwapchainInfo().scissor});

        vulkanRHI->CommandDraw(vulkanRHI->GetCurrentCommandBuffer(), 3, 1, 0, 0);

        vulkanRHI->CommandEndRenderPass(vulkanRHI->GetCurrentCommandBuffer());
    }

    void MainCameraPass::SetupAttachment()
    {
        framebufferAttachments.resize(eMainCameraPassCustomAttachmentCount);

        framebufferAttachments[eMainCameraPassAttachmentGBufferA].format = vk::Format::eR8G8B8A8Unorm;
        framebufferAttachments[eMainCameraPassAttachmentGBufferB].format = vk::Format::eR8G8B8A8Unorm;
        framebufferAttachments[eMainCameraPassAttachmentGBufferC].format = vk::Format::eR8G8B8A8Unorm;

        for(int i = 0; i < eMainCameraPassCustomAttachmentCount; i++)
        {
            vk::ImageCreateInfo imageCreateInfo;
            imageCreateInfo.setImageType(vk::ImageType::e2D)
                .setArrayLayers(1)
                .setMipLevels(1)
                .setExtent({vulkanRHI->GetSwapchainInfo().extent.width, vulkanRHI->GetSwapchainInfo().extent.height, 1})
                .setFormat(framebufferAttachments[i].format)
                .setTiling(vk::ImageTiling::eOptimal)
                .setInitialLayout(vk::ImageLayout::eUndefined)
                .setUsage(vk::ImageUsageFlagBits::eInputAttachment |
                          vk::ImageUsageFlagBits::eColorAttachment |
                          vk::ImageUsageFlagBits::eTransientAttachment)
                .setSamples(vk::SampleCountFlagBits::e1);

            framebufferAttachments[i].image = vulkanRHI->CreateImage(imageCreateInfo, vk::MemoryPropertyFlagBits::eDeviceLocal, framebufferAttachments[i].memory);

            vk::ImageSubresourceRange range;
            range.setAspectMask(vk::ImageAspectFlagBits::eColor)
                .setBaseArrayLayer(0)
                .setLayerCount(1)
                .setLevelCount(1)
                .setBaseMipLevel(0);

            vk::ImageViewCreateInfo imageViewCreateInfo;
            imageViewCreateInfo.setImage(framebufferAttachments[i].image)
                .setViewType(vk::ImageViewType::e2D)
                .setFormat(framebufferAttachments[i].format)
                .setSubresourceRange(range);

            framebufferAttachments[i].imageView = vulkanRHI->CreateImageView(imageViewCreateInfo);
        }
    }

    void MainCameraPass::SetupRenderPass()
    {
        // Attachment Description
        std::vector<vk::AttachmentDescription> attachmentDescriptions(eMainCameraPassAttachmentCount);

        vk::AttachmentDescription &gBufferNormalAttachmentDescription = attachmentDescriptions[eMainCameraPassAttachmentGBufferA];
        gBufferNormalAttachmentDescription.setFormat(framebufferAttachments[eMainCameraPassAttachmentGBufferA].format)
            .setSamples(vk::SampleCountFlagBits::e1)
            .setInitialLayout(vk::ImageLayout::eUndefined)
            .setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
            .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);

        vk::AttachmentDescription &gBufferMetallicRoughnessSpecluarAttachmentDescription = attachmentDescriptions[eMainCameraPassAttachmentGBufferB];
        gBufferMetallicRoughnessSpecluarAttachmentDescription.setFormat(framebufferAttachments[eMainCameraPassAttachmentGBufferB].format)
            .setSamples(vk::SampleCountFlagBits::e1)
            .setInitialLayout(vk::ImageLayout::eUndefined)
            .setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
            .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);

        vk::AttachmentDescription &gBufferAlbedoAttachmentDescription = attachmentDescriptions[eMainCameraPassAttachmentGBufferC];
        gBufferAlbedoAttachmentDescription.setFormat(framebufferAttachments[eMainCameraPassAttachmentGBufferC].format)
            .setSamples(vk::SampleCountFlagBits::e1)
            .setInitialLayout(vk::ImageLayout::eUndefined)
            .setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
            .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);

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

        std::vector<vk::AttachmentReference> basePassColorAttachmentReferences(3);
        basePassColorAttachmentReferences[0].setAttachment(eMainCameraPassAttachmentGBufferA)
            .setLayout(vk::ImageLayout::eColorAttachmentOptimal);
        basePassColorAttachmentReferences[1].setAttachment(eMainCameraPassAttachmentGBufferB)
            .setLayout(vk::ImageLayout::eColorAttachmentOptimal);
        basePassColorAttachmentReferences[2].setAttachment(eMainCameraPassAttachmentGBufferC)
            .setLayout(vk::ImageLayout::eColorAttachmentOptimal);

        vk::AttachmentReference basePassDepthAttachmentReference;
        basePassDepthAttachmentReference.setAttachment(eMainCameraPassAttachmentDepth)
            .setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

        vk::SubpassDescription &basePass = subpasses[eMainCameraSubpassBasePass];
        basePass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
            .setColorAttachments(basePassColorAttachmentReferences)
            .setPDepthStencilAttachment(&basePassDepthAttachmentReference);

        std::vector<vk::AttachmentReference> deferredLightingInputAttachmentReferences(4);
        deferredLightingInputAttachmentReferences[0].setAttachment(eMainCameraPassAttachmentGBufferA)
            .setLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
        deferredLightingInputAttachmentReferences[1].setAttachment(eMainCameraPassAttachmentGBufferB)
            .setLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
        deferredLightingInputAttachmentReferences[2].setAttachment(eMainCameraPassAttachmentGBufferC)
            .setLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
        deferredLightingInputAttachmentReferences[3].setAttachment(eMainCameraPassAttachmentDepth)
            .setLayout(vk::ImageLayout::eShaderReadOnlyOptimal);

        vk::AttachmentReference deferredLightingColorAttachmentReference;
        deferredLightingColorAttachmentReference.setAttachment(eMainCameraPassAttachmentSwapchainImage)
            .setLayout(vk::ImageLayout::eColorAttachmentOptimal);

        vk::SubpassDescription &deferredLightingPass = subpasses[eMainCameraSubpassDeferredLighting];
        deferredLightingPass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
            .setInputAttachments(deferredLightingInputAttachmentReferences)
            .setColorAttachments(deferredLightingColorAttachmentReference);

        // Dependencies
        std::vector<vk::SubpassDependency> dependencies(2);

        vk::SubpassDependency &deferredLightingDependOnExternalDependency = dependencies[0];
        deferredLightingDependOnExternalDependency.setSrcSubpass(VK_SUBPASS_EXTERNAL)
            .setDstSubpass(eMainCameraSubpassDeferredLighting)
            .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eLateFragmentTests)
            .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests)
            .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite)
            .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite);
        vk::SubpassDependency &deferredLightingDependOnBasePassDependency = dependencies[1];
        deferredLightingDependOnBasePassDependency.setSrcSubpass(eMainCameraSubpassBasePass)
            .setDstSubpass(eMainCameraSubpassDeferredLighting)
            .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eFragmentShader)
            .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eFragmentShader)
            .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eShaderWrite)
            .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eShaderRead);

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

        // mesh gbuffer
        {
            vk::ShaderModule vertShader = vulkanRHI->CreateShaderModule(MESH_VERT);
            vk::ShaderModule fragShader = vulkanRHI->CreateShaderModule(MESH_GBUFFER_FRAG);

            // layout
            vk::PipelineLayoutCreateInfo layoutCreateInfo;
            std::vector<vk::DescriptorSetLayout> pipelineDescriptorSetLayouts = {descriptorSetLayouts[eDescriptorSetLayoutPerFrame],
                                                                                 descriptorSetLayouts[eDescriptorSetLayoutMeshPerMaterial]}; 
            layoutCreateInfo.setSetLayouts(pipelineDescriptorSetLayouts);
            renderPipelines[eRenderPipelineTypeMeshGBuffer].layout = vulkanRHI->CreatePipelineLayout(layoutCreateInfo);

            // shader
            vk::PipelineShaderStageCreateInfo vertShaderStageCreateInfo;
            vertShaderStageCreateInfo.setStage(vk::ShaderStageFlagBits::eVertex)
                .setModule(vertShader)
                .setPName("main");
            vk::PipelineShaderStageCreateInfo fragShaderStageCreateIngo;
            fragShaderStageCreateIngo.setStage(vk::ShaderStageFlagBits::eFragment)
                .setModule(fragShader)
                .setPName("main");
            std::vector<vk::PipelineShaderStageCreateInfo> shaderStages{vertShaderStageCreateInfo, fragShaderStageCreateIngo};

            // vertex input
            vk::PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo;
            std::vector<vk::VertexInputAttributeDescription> attributeDescription = MeshVertex::GetAttributeDescriptions();
            std::vector<vk::VertexInputBindingDescription> bindingDescriptions = MeshVertex::GetBindingDescriptions();
            vertexInputStateCreateInfo.setVertexAttributeDescriptions(attributeDescription)
                .setVertexBindingDescriptions(bindingDescriptions);

            // input assembly
            vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo;
            inputAssemblyStateCreateInfo.setPrimitiveRestartEnable(false)
                .setTopology(vk::PrimitiveTopology::eTriangleList);

            // viewport
            vk::PipelineViewportStateCreateInfo viewportStateCreateInfo;
            vk::Viewport viewport = vulkanRHI->GetSwapchainInfo().viewport;
            vk::Rect2D scissor = vulkanRHI->GetSwapchainInfo().scissor;
            viewportStateCreateInfo.setViewports(viewport)
                .setScissors(scissor);

            // rasterization
            vk::PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo;
            rasterizationStateCreateInfo.setRasterizerDiscardEnable(false)
                .setCullMode(vk::CullModeFlagBits::eBack)
                .setFrontFace(vk::FrontFace::eCounterClockwise)
                .setPolygonMode(vk::PolygonMode::eFill)
                .setLineWidth(1.0f);

            // multisample
            vk::PipelineMultisampleStateCreateInfo multisampleStateCreateInfo;
            multisampleStateCreateInfo.setSampleShadingEnable(false)
                .setRasterizationSamples(vk::SampleCountFlagBits::e1);

            // color blending
            std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachmentStates(3);
            colorBlendAttachmentStates[0].setBlendEnable(false)
                .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
            colorBlendAttachmentStates[1].setBlendEnable(false)
                .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
            colorBlendAttachmentStates[2].setBlendEnable(false)
                .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);

            vk::PipelineColorBlendStateCreateInfo colorBlendCreateInfo;
            colorBlendCreateInfo.setLogicOpEnable(false)
                .setAttachments(colorBlendAttachmentStates);

            // depth and stencil test
            vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo;
            depthStencilStateCreateInfo.setDepthTestEnable(true)
                .setDepthWriteEnable(true)
                .setDepthCompareOp(vk::CompareOp::eLess)
                .setDepthBoundsTestEnable(false)
                .setStencilTestEnable(false);

            // dynamic state
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
                .setLayout(renderPipelines[eRenderPipelineTypeMeshGBuffer].layout);

            renderPipelines[eRenderPipelineTypeMeshGBuffer].pipeline = vulkanRHI->CreateGraphicsPipeline(createInfo);

            vulkanRHI->DestroyShaderModule(vertShader);
            vulkanRHI->DestroyShaderModule(fragShader);
        }

        // deferred lighting
        {
            vk::ShaderModule vertShader = vulkanRHI->CreateShaderModule(DEFERRED_LIGHTING_VERT);
            vk::ShaderModule fragShader = vulkanRHI->CreateShaderModule(DEFERRED_LIGHTING_FRAG);

            // layout
            vk::PipelineLayoutCreateInfo layoutCreateInfo;
            std::vector<vk::DescriptorSetLayout> pipelineDescriptorSetLayouts = {descriptorSetLayouts[eDescriptorSetLayoutPerFrame],
                                                                                 descriptorSetLayouts[eDescriptorSetLayoutDeferredLighting]}; 
            layoutCreateInfo.setSetLayouts(pipelineDescriptorSetLayouts);
            renderPipelines[eRenderPipelineTypeDeferredLighting].layout = vulkanRHI->CreatePipelineLayout(layoutCreateInfo);

            // shader
            vk::PipelineShaderStageCreateInfo vertShaderStageCreateInfo;
            vertShaderStageCreateInfo.setStage(vk::ShaderStageFlagBits::eVertex)
                .setModule(vertShader)
                .setPName("main");
            vk::PipelineShaderStageCreateInfo fragShaderStageCreateIngo;
            fragShaderStageCreateIngo.setStage(vk::ShaderStageFlagBits::eFragment)
                .setModule(fragShader)
                .setPName("main");
            std::vector<vk::PipelineShaderStageCreateInfo> shaderStages{vertShaderStageCreateInfo, fragShaderStageCreateIngo};

            // vertex input
            vk::PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo;
            vertexInputStateCreateInfo.setVertexAttributeDescriptions(NULL)
                .setVertexBindingDescriptions(NULL);

            // input assembly
            vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo;
            inputAssemblyStateCreateInfo.setPrimitiveRestartEnable(false)
                .setTopology(vk::PrimitiveTopology::eTriangleList);

            // viewport
            vk::PipelineViewportStateCreateInfo viewportStateCreateInfo;
            vk::Viewport viewport = vulkanRHI->GetSwapchainInfo().viewport;
            vk::Rect2D scissor = vulkanRHI->GetSwapchainInfo().scissor;
            viewportStateCreateInfo.setViewports(viewport)
                .setScissors(scissor);

            // rasterization
            vk::PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo;
            rasterizationStateCreateInfo.setRasterizerDiscardEnable(false)
                .setCullMode(vk::CullModeFlagBits::eBack)
                .setFrontFace(vk::FrontFace::eCounterClockwise)
                .setPolygonMode(vk::PolygonMode::eFill)
                .setLineWidth(1.0f);

            // multisample
            vk::PipelineMultisampleStateCreateInfo multisampleStateCreateInfo;
            multisampleStateCreateInfo.setSampleShadingEnable(false)
                .setRasterizationSamples(vk::SampleCountFlagBits::e1);

            // color blending
            vk::PipelineColorBlendAttachmentState colorBlendAttachmentState;
            colorBlendAttachmentState.setBlendEnable(false)
                .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);

            vk::PipelineColorBlendStateCreateInfo colorBlendCreateInfo;
            colorBlendCreateInfo.setLogicOpEnable(false)
                .setAttachments(colorBlendAttachmentState);

            // depth and stencil test
            vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo;
            depthStencilStateCreateInfo.setDepthTestEnable(false)
                .setDepthWriteEnable(false)
                .setDepthCompareOp(vk::CompareOp::eAlways)
                .setDepthBoundsTestEnable(false)
                .setStencilTestEnable(false);

            // dynamic state
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
                .setSubpass(eMainCameraSubpassDeferredLighting)
                .setLayout(renderPipelines[eRenderPipelineTypeDeferredLighting].layout);

            renderPipelines[eRenderPipelineTypeDeferredLighting].pipeline = vulkanRHI->CreateGraphicsPipeline(createInfo);

            vulkanRHI->DestroyShaderModule(vertShader);
            vulkanRHI->DestroyShaderModule(fragShader);
        }
    }

    void MainCameraPass::SetupSwapchainFramebuffers()
    {
        swapchainFramebuffers.resize(vulkanRHI->GetSwapchainImageViews().size());

        for (int i = 0; i < swapchainFramebuffers.size(); i++)
        {
            std::vector<vk::ImageView> framebufferAttachmentsImageViews{framebufferAttachments[eMainCameraPassAttachmentGBufferA].imageView,
                                                                        framebufferAttachments[eMainCameraPassAttachmentGBufferB].imageView,
                                                                        framebufferAttachments[eMainCameraPassAttachmentGBufferC].imageView,
                                                                        vulkanRHI->GetSwapchainImageViews()[i],
                                                                        vulkanRHI->GetDepthImageInfo().imageView};

            vk::FramebufferCreateInfo createInfo;
            createInfo.setAttachments(framebufferAttachmentsImageViews)
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

        {
            std::vector<vk::DescriptorSetLayoutBinding> deferredLightingBindings;
            deferredLightingBindings.resize(4);

            deferredLightingBindings[0].setBinding(0)
                .setDescriptorType(vk::DescriptorType::eInputAttachment)
                .setDescriptorCount(1)
                .setStageFlags(vk::ShaderStageFlagBits::eFragment);
                
            deferredLightingBindings[1].setBinding(1)
                .setDescriptorType(vk::DescriptorType::eInputAttachment)
                .setDescriptorCount(1)
                .setStageFlags(vk::ShaderStageFlagBits::eFragment);
                
            deferredLightingBindings[2].setBinding(2)
                .setDescriptorType(vk::DescriptorType::eInputAttachment)
                .setDescriptorCount(1)
                .setStageFlags(vk::ShaderStageFlagBits::eFragment);
                
            deferredLightingBindings[3].setBinding(3)
                .setDescriptorType(vk::DescriptorType::eInputAttachment)
                .setDescriptorCount(1)
                .setStageFlags(vk::ShaderStageFlagBits::eFragment);

            vk::DescriptorSetLayoutCreateInfo createInfo;
            createInfo.setBindings(deferredLightingBindings);

            descriptorSetLayouts[eDescriptorSetLayoutDeferredLighting] = vulkanRHI->CreateDescriptorSetLayout(createInfo);
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
            vk::DescriptorImageInfo AOImageInfo;
            AOImageInfo.setSampler(sampler)
                .setImageView(renderResource->material.AO.imageView)
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
                .setImageInfo(AOImageInfo)
                .setDstSet(descriptorSets[eDescriptorSetLayoutMeshPerMaterial])
                .setDstBinding(4)
                .setDescriptorCount(1);

            vulkanRHI->UpdateDescriptorSets(writes, {});
        }

        {
            vk::DescriptorSetAllocateInfo allocateInfo;
            allocateInfo.setDescriptorPool(vulkanRHI->GetDescriptorPool())
                .setDescriptorSetCount(1)
                .setSetLayouts(descriptorSetLayouts[eDescriptorSetLayoutDeferredLighting]);

            descriptorSets[eDescriptorSetLayoutDeferredLighting] = vulkanRHI->AllocateDescriptorSets(allocateInfo);

            vk::DescriptorImageInfo gBufferAImageInfo;
            gBufferAImageInfo.setImageView(framebufferAttachments[eMainCameraPassAttachmentGBufferA].imageView)
                .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
            vk::DescriptorImageInfo gBufferBImageInfo;
            gBufferBImageInfo.setImageView(framebufferAttachments[eMainCameraPassAttachmentGBufferB].imageView)
                .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
            vk::DescriptorImageInfo gBufferCImageInfo;
            gBufferCImageInfo.setImageView(framebufferAttachments[eMainCameraPassAttachmentGBufferC].imageView)
                .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
            vk::DescriptorImageInfo depthImageInfo;
            depthImageInfo.setImageView(vulkanRHI->GetDepthImageInfo().imageView)
                .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);

            std::vector<vk::WriteDescriptorSet> writes;
            writes.resize(4);

            writes[0].setDescriptorType(vk::DescriptorType::eInputAttachment)
                .setImageInfo(gBufferAImageInfo)
                .setDstSet(descriptorSets[eDescriptorSetLayoutDeferredLighting])
                .setDstBinding(0)
                .setDescriptorCount(1);
            writes[1].setDescriptorType(vk::DescriptorType::eInputAttachment)
                .setImageInfo(gBufferBImageInfo)
                .setDstSet(descriptorSets[eDescriptorSetLayoutDeferredLighting])
                .setDstBinding(1)
                .setDescriptorCount(1);
            writes[2].setDescriptorType(vk::DescriptorType::eInputAttachment)
                .setImageInfo(gBufferCImageInfo)
                .setDstSet(descriptorSets[eDescriptorSetLayoutDeferredLighting])
                .setDstBinding(2)
                .setDescriptorCount(1);
            writes[3].setDescriptorType(vk::DescriptorType::eInputAttachment)
                .setImageInfo(depthImageInfo)
                .setDstSet(descriptorSets[eDescriptorSetLayoutDeferredLighting])
                .setDstBinding(3)
                .setDescriptorCount(1);

            vulkanRHI->UpdateDescriptorSets(writes, {});
        }
    }
}