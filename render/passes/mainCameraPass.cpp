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
        
        sampler = vulkanRHI->CreateDefaultLinearSampler();
    }

    void MainCameraPass::Quit()
    {
        vulkanRHI->DestroyRenderPass(renderPass);
        vulkanRHI->DestroyDescriptorSetLayout(descriptorSetLayout);

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

    void MainCameraPass::PreparePassData(const vk::Buffer &uniformBuffer)
    {
        this->uniformBuffer = uniformBuffer;

        SetupDescriptorSet();
    }

    void MainCameraPass::Draw()
    {
        vk::Rect2D renderArea;
        renderArea.setOffset({0, 0})
            .setExtent(vulkanRHI->GetSwapchainInfo().extent);

        std::vector<vk::ClearValue> clearValues(eMainCameraPassAttachmentCount);
        clearValues[eMainCameraPassAttachmentSwapchainImage].setColor({0.0f, 0.0f, 0.0f, 0.0f});

        vk::RenderPassBeginInfo beginInfo;
        beginInfo.setRenderPass(renderPass)
            .setFramebuffer(swapchainFramebuffers[vulkanRHI->GetCurrentSwapchainImageIndex()])
            .setRenderArea(renderArea)
            .setClearValues(clearValues);

        vulkanRHI->CommandBeginRenderPass(vulkanRHI->GetCurrentCommandBuffer(), beginInfo, vk::SubpassContents::eInline);

        vulkanRHI->CommandBindDescriptorSet(vulkanRHI->GetCurrentCommandBuffer(), vk::PipelineBindPoint::eGraphics, renderPipelines[eRenderPipelineTypeMesh].layout, 0, {descriptorSet}, {});

        vulkanRHI->CommandBindPipeline(vulkanRHI->GetCurrentCommandBuffer(), vk::PipelineBindPoint::eGraphics, renderPipelines[eRenderPipelineTypeMesh].pipeline);

        vulkanRHI->CommandBindVertexBuffers(vulkanRHI->GetCurrentCommandBuffer(), 0, {renderResource->mesh.vertexBuffer}, {0});

        vulkanRHI->CommandBindIndexBuffer(vulkanRHI->GetCurrentCommandBuffer(), renderResource->mesh.indexBuffer, 0, vk::IndexType::eUint16);

        vulkanRHI->CommandDrawIndexed(vulkanRHI->GetCurrentCommandBuffer(), 6, 1, 0, 0, 0);

        vulkanRHI->CommandEndRenderPass(vulkanRHI->GetCurrentCommandBuffer());
    }

    void MainCameraPass::SetupRenderPass()
    {
        // Attachment Description
        std::vector<vk::AttachmentDescription> attachmentDescription(eMainCameraPassAttachmentCount);

        vk::AttachmentDescription &swapchainImageAttachmentDescription = attachmentDescription[eMainCameraPassAttachmentSwapchainImage];
        swapchainImageAttachmentDescription.setFormat(vulkanRHI->GetSwapchainInfo().format)
            .setSamples(vk::SampleCountFlagBits::e1)
            .setInitialLayout(vk::ImageLayout::eUndefined)
            .setFinalLayout(vk::ImageLayout::ePresentSrcKHR)
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
            .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);

        // Subpass
        std::vector<vk::SubpassDescription> subpasses(eMainCameraSubpassCount);

        vk::AttachmentReference basePassColorAttachmentReference;
        basePassColorAttachmentReference.setAttachment(eMainCameraPassAttachmentSwapchainImage)
            .setLayout(vk::ImageLayout::eColorAttachmentOptimal);

        vk::SubpassDescription &basePass = subpasses[eMainCameraSubpassBasePass];
        basePass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
            .setColorAttachments(basePassColorAttachmentReference);

        // Dependencies
        std::vector<vk::SubpassDependency> dependencies(1);

        vk::SubpassDependency &basePassDependency = dependencies[0];
        basePassDependency.setSrcSubpass(VK_SUBPASS_EXTERNAL)
            .setDstSubpass(eMainCameraSubpassBasePass)
            .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
            .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
            .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
            .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead);

        // RenderPass
        vk::RenderPassCreateInfo createInfo;
        createInfo.setAttachments(attachmentDescription)
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
        layoutCreateInfo.setSetLayouts(descriptorSetLayout);
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
        depthStencilStateCreateInfo.setDepthTestEnable(false)
            .setDepthWriteEnable(false)
            .setDepthCompareOp(vk::CompareOp::eLess)
            .setDepthBoundsTestEnable(false)
            .setStencilTestEnable(false);

        // TODO:: 9. Dynamic state (Maybe)

        vk::GraphicsPipelineCreateInfo createInfo;
        createInfo.setPVertexInputState(&vertexInputStateCreateInfo)
            .setPInputAssemblyState(&inputAssemblyStateCreateInfo)
            .setStages(shaderStages)
            .setPViewportState(&viewportStateCreateInfo)
            .setPRasterizationState(&rasterizationStateCreateInfo)
            .setPMultisampleState(&multisampleStateCreateInfo)
            .setPColorBlendState(&colorBlendCreateInfo)
            .setPDepthStencilState(&depthStencilStateCreateInfo)
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
            vk::FramebufferCreateInfo createInfo;
            createInfo.setAttachments(vulkanRHI->GetSwapchainImageViews()[i])
                .setWidth(vulkanRHI->GetSwapchainInfo().extent.width)
                .setHeight(vulkanRHI->GetSwapchainInfo().extent.height)
                .setRenderPass(renderPass)
                .setLayers(1);

            swapchainFramebuffers[i] = vulkanRHI->CreateFramebuffer(createInfo);
        }
    }

    void MainCameraPass::SetupDescriptorSetLayout()
    {
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
        bindings.resize(2);

        bindings[0].setBinding(0)
            .setDescriptorType(vk::DescriptorType::eUniformBuffer)
            .setDescriptorCount(1)
            .setStageFlags(vk::ShaderStageFlagBits::eFragment);
        bindings[1].setBinding(1)
            .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
            .setDescriptorCount(1)
            .setStageFlags(vk::ShaderStageFlagBits::eFragment);

        vk::DescriptorSetLayoutCreateInfo createInfo;
        createInfo.setBindings(bindings);

        descriptorSetLayout = vulkanRHI->CreateDescriptorSetLayout(createInfo);
    }

    void MainCameraPass::SetupDescriptorSet()
    {
        vk::DescriptorSetAllocateInfo allocateInfo;
        allocateInfo.setDescriptorPool(vulkanRHI->GetDescriptorPool())
            .setDescriptorSetCount(1)
            .setSetLayouts(descriptorSetLayout);

        descriptorSet = vulkanRHI->AllocateDescriptorSets(allocateInfo);

        vk::DescriptorBufferInfo bufferInfo;
        bufferInfo.setBuffer(uniformBuffer)
            .setOffset(0)
            .setRange(sizeof(glm::vec4));

        vk::DescriptorImageInfo imageInfo;
        imageInfo.setSampler(sampler)
            .setImageView(renderResource->texture.imageView)
            .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);

        std::vector<vk::WriteDescriptorSet> writes;
        writes.resize(2);

        writes[0].setDescriptorType(vk::DescriptorType::eUniformBuffer)
            .setBufferInfo(bufferInfo)
            .setDstSet(descriptorSet)
            .setDstBinding(0)
            .setDescriptorCount(1);
        writes[1].setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
            .setImageInfo(imageInfo)
            .setDstSet(descriptorSet)
            .setDstBinding(1)
            .setDescriptorCount(1);

        vulkanRHI->UpdateDescriptorSets(writes, {});
    }
}