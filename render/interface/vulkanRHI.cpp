#include "render/interface/vulkanRHI.hpp"

namespace XrayEngine
{
    void VulkanRHI::Initialize(const VulkanRHIInitInfo &initInfo)
    {
        window = initInfo.window;
        viewport = vk::Viewport{0.0f, 0.0f, initInfo.windowWidth, initInfo.windowHeight, 0.0f, 1.0f};
        scissor = vk::Rect2D{{0, 0}, {(uint32_t)initInfo.windowWidth, (uint32_t)initInfo.windowHeight}};
        CreateInstance();
        InitializePhysicalDevice();
        CreateWindowSurface();
        CreateLogicalDevice();
        CreateCommandPool();
        AllocateCommandBuffers();
        CreateDescriptorPool();
        CreateSemaphores();
        CreateFences();
        CreateSwapchain();
        CreateSwapchainImageViews();
    }

    void VulkanRHI::Quit()
    {
        device.destroyCommandPool(commandPool);
        for (int i = 0; i < maxFramesInFlight; i++)
        {
            device.destroySemaphore(imageAvailableSemaphores[i]);
            device.destroySemaphore(renderFinishedSemaphores[i]);
            device.destroyFence(inFlightFences[i]);
            device.destroyCommandPool(commandPools[i]);
        }
        for (const vk::ImageView &imageView : swapchainImageViews)
        {
            device.destroyImageView(imageView);
        }
        device.destroyDescriptorPool(descriptorPool);
        device.destroySwapchainKHR(swapchain);
        instance.destroySurfaceKHR(surface);
        device.destroy();
        instance.destroy();
    }

    void VulkanRHI::CreateInstance()
    {
        const std::vector<const char *> layers{"VK_LAYER_KHRONOS_validation"};
        const std::vector<const char *> extensions = GetRequiredExtensions();

        vk::ApplicationInfo appInfo;
        appInfo.setApiVersion(VK_API_VERSION_1_4);

        vk::InstanceCreateInfo createInfo;
        createInfo.setPApplicationInfo(&appInfo)
            .setPEnabledLayerNames(layers)
            .setPEnabledExtensionNames(extensions);
        instance = vk::createInstance(createInfo);
    }

    void VulkanRHI::InitializePhysicalDevice()
    {
        std::vector<vk::PhysicalDevice> physicalDevices;
        physicalDevices = instance.enumeratePhysicalDevices();

        std::vector<std::pair<int, vk::PhysicalDevice>> rankedPhysicalDevices;
        for (const vk::PhysicalDevice &device : physicalDevices)
        {
            vk::PhysicalDeviceProperties physicalDeviceProperties;
            device.getProperties(&physicalDeviceProperties);
            int score = 0;

            if (physicalDeviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
            {
                score += 1000;
            }
            else if (physicalDeviceProperties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu)
            {
                score += 100;
            }

            rankedPhysicalDevices.push_back({score, device});
        }

        std::sort(rankedPhysicalDevices.begin(),
                  rankedPhysicalDevices.end(),
                  [](const std::pair<int, vk::PhysicalDevice> &p1, const std::pair<int, vk::PhysicalDevice> &p2)
                  {
                      return p1 > p2;
                  });

        // TODO:: Add physical device suitable check.

        physicalDevice = rankedPhysicalDevices[0].second;
    }

    void VulkanRHI::CreateLogicalDevice()
    {
        queueFamilyIndices = QueryQueueFamilies();

        std::vector<vk::DeviceQueueCreateInfo> deviceQueueCreateInfos;
        std::set<uint32_t> queueIndices{queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentFamily.value(), queueFamilyIndices.computeFamily.value()};

        const std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
        float priorities = 1.0f;
        for (uint32_t queueIndex : queueIndices)
        {
            vk::DeviceQueueCreateInfo deviceQueueCreateInfo;
            deviceQueueCreateInfo.setQueueCount(1)
                .setPQueuePriorities(&priorities)
                .setQueueFamilyIndex(queueIndex);
            deviceQueueCreateInfos.push_back(deviceQueueCreateInfo);
        }

        vk::DeviceCreateInfo createInfo;
        createInfo.setPQueueCreateInfos(deviceQueueCreateInfos.data())
            .setQueueCreateInfoCount(static_cast<uint32_t>(deviceQueueCreateInfos.size()))
            .setEnabledExtensionCount(static_cast<uint32_t>(deviceExtensions.size()))
            .setPEnabledExtensionNames(deviceExtensions);
        device = physicalDevice.createDevice(createInfo);

        graphicsQueue = device.getQueue(queueFamilyIndices.graphicsFamily.value(), 0);
        presentQueue = device.getQueue(queueFamilyIndices.presentFamily.value(), 0);
    }

    void VulkanRHI::CreateWindowSurface()
    {
        VkSurfaceKHR c_surface;
        glfwCreateWindowSurface(instance, window, nullptr, &c_surface);
        surface = vk::SurfaceKHR(c_surface);
    }

    void VulkanRHI::CreateSwapchain()
    {
        SwapchainSupportDetails swapchainSupportDetails = QuerySwapchainSupport();
        vk::SurfaceFormatKHR surfaceFormat = swapchainSupportDetails.surfaceFormat;
        vk::PresentModeKHR presentMode = swapchainSupportDetails.presentMode;
        vk::Extent2D chosenExtent = ChooseSwapchainExtent(swapchainSupportDetails.capabilities);
        uint32_t imageCount = swapchainSupportDetails.capabilities.minImageCount + 1;
        if (swapchainSupportDetails.capabilities.maxImageCount > 0 && imageCount > swapchainSupportDetails.capabilities.maxImageCount)
        {
            imageCount = swapchainSupportDetails.capabilities.maxImageCount;
        }

        vk::SwapchainCreateInfoKHR createInfo;
        createInfo.setSurface(surface)
            .setClipped(true)
            .setImageArrayLayers(1)
            .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
            .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
            .setMinImageCount(imageCount)
            .setImageExtent(chosenExtent)
            .setPreTransform(swapchainSupportDetails.capabilities.currentTransform)
            .setImageFormat(surfaceFormat.format)
            .setImageColorSpace(surfaceFormat.colorSpace)
            .setPresentMode(presentMode);

        if (queueFamilyIndices.graphicsFamily != queueFamilyIndices.presentFamily)
        {
            uint32_t queueIndices[] = {queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentFamily.value()};
            createInfo.setImageSharingMode(vk::SharingMode::eConcurrent)
                .setQueueFamilyIndexCount(2)
                .setPQueueFamilyIndices(queueIndices);
        }
        else
        {
            createInfo.setImageSharingMode(vk::SharingMode::eExclusive);
        }

        swapchain = device.createSwapchainKHR(createInfo);

        swapchainImages = device.getSwapchainImagesKHR(swapchain);
        extent = chosenExtent;
        format = swapchainSupportDetails.surfaceFormat.format;
    }

    void VulkanRHI::CreateSwapchainImageViews()
    {
        swapchainImageViews.resize(swapchainImages.size());

        for (int i = 0; i < swapchainImageViews.size(); i++)
        {
            vk::ComponentMapping mapping;
            vk::ImageSubresourceRange subresourceRange;
            subresourceRange.setBaseMipLevel(0)
                .setLevelCount(1)
                .setBaseArrayLayer(0)
                .setLayerCount(1)
                .setAspectMask(vk::ImageAspectFlagBits::eColor);

            vk::ImageViewCreateInfo createInfo;
            createInfo.setImage(swapchainImages[i])
                .setViewType(vk::ImageViewType::e2D)
                .setComponents(mapping)
                .setFormat(format)
                .setSubresourceRange(subresourceRange);
            swapchainImageViews[i] = device.createImageView(createInfo);
        }
    }

    void VulkanRHI::CreateCommandPool()
    {
        vk::CommandPoolCreateInfo createInfo;
        createInfo.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
            .setQueueFamilyIndex(queueFamilyIndices.graphicsFamily.value());
        commandPool = device.createCommandPool(createInfo);
        
        createInfo.setFlags(vk::CommandPoolCreateFlagBits::eTransient)
            .setQueueFamilyIndex(queueFamilyIndices.graphicsFamily.value());
        // TODO:: Error check
        for (int i = 0; i < maxFramesInFlight; i++)
        {
            commandPools[i] = device.createCommandPool(createInfo);
        }
    }

    void VulkanRHI::CreateSemaphores()
    {
        vk::SemaphoreCreateInfo createInfo;
        for (int i = 0; i < maxFramesInFlight; i++)
        {
            imageAvailableSemaphores[i] = device.createSemaphore(createInfo);
            renderFinishedSemaphores[i] = device.createSemaphore(createInfo);
        }
    }

    void VulkanRHI::CreateFences()
    {
        vk::FenceCreateInfo createInfo;
        createInfo.setFlags(vk::FenceCreateFlagBits::eSignaled);
        for (int i = 0; i < maxFramesInFlight; i++)
        {
            inFlightFences[i] = device.createFence(createInfo);
        }
    }

    void VulkanRHI::CreateDescriptorPool()
    {
        std::vector<vk::DescriptorPoolSize> poolSizes;
        poolSizes.resize(2);

        poolSizes[0].setType(vk::DescriptorType::eUniformBuffer)
            .setDescriptorCount(1);
        poolSizes[1].setType(vk::DescriptorType::eCombinedImageSampler)
            .setDescriptorCount(1);

        vk::DescriptorPoolCreateInfo createInfo;
        createInfo.setPoolSizes(poolSizes)
            .setMaxSets(1);

        // TODO:: ERROR CHECK
        descriptorPool = device.createDescriptorPool(createInfo);
    }

    void VulkanRHI::AllocateCommandBuffers()
    {
        vk::CommandBufferAllocateInfo allocateInfo;
        allocateInfo.setCommandBufferCount(1)
            .setLevel(vk::CommandBufferLevel::ePrimary);

        for (int i = 0; i < maxFramesInFlight; i++)
        {
            allocateInfo.setCommandPool(commandPools[i]);

            // TODO:: Error check
            vk::CommandBuffer commandBuffer = device.allocateCommandBuffers(allocateInfo)[0];
            commandBuffers[i] = commandBuffer;
        }
    }

    void VulkanRHI::FreeCommandBuffers()
    {
        for (int i = 0; i < maxFramesInFlight; i++)
        {
            device.freeCommandBuffers(commandPools[i], commandBuffers[i]);
        }
    }

    std::vector<const char *> VulkanRHI::GetRequiredExtensions()
    {
        uint32_t glfwExtensionCount = 0;
        const char **glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        return extensions;
    }

    QueueFamilyIndices VulkanRHI::QueryQueueFamilies()
    {
        std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
        QueueFamilyIndices queueIndices;

        int i = 0;
        for (const vk::QueueFamilyProperties &queueFamilyProperty : queueFamilyProperties)
        {
            if (queueFamilyProperty.queueFlags & vk::QueueFlagBits::eGraphics)
            {
                queueIndices.graphicsFamily = i;
            }
            if (queueFamilyProperty.queueFlags & vk::QueueFlagBits::eCompute)
            {
                queueIndices.computeFamily = i;
            }

            vk::Bool32 isPresentSupport = false;

            vk::Result result = physicalDevice.getSurfaceSupportKHR(i, surface, &isPresentSupport);

            if (result != vk::Result::eSuccess)
            {
                // TODO:: FATAL ERROR LOG
                throw std::runtime_error("Failed to query surface support!");
            }

            if (isPresentSupport)
            {
                queueIndices.presentFamily = i;
            }

            if (queueIndices.isComplete())
            {
                break;
            }
            i++;
        }
        return queueIndices;
    }

    SwapchainSupportDetails VulkanRHI::QuerySwapchainSupport()
    {
        SwapchainSupportDetails details;

        details.capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);

        std::vector<vk::SurfaceFormatKHR> formats = physicalDevice.getSurfaceFormatsKHR(surface);
        details.surfaceFormat = formats[0];
        for (const vk::SurfaceFormatKHR &format : formats)
        {
            if (format.format == vk::Format::eR8G8B8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
            {
                details.surfaceFormat = format;
                break;
            }
        }

        std::vector<vk::PresentModeKHR> presentModes = physicalDevice.getSurfacePresentModesKHR(surface);
        details.presentMode = vk::PresentModeKHR::eFifo;
        for (const vk::PresentModeKHR &presentMode : presentModes)
        {
            if (presentMode == vk::PresentModeKHR::eMailbox)
            {
                details.presentMode = presentMode;
                break;
            }
        }

        return details;
    }

    uint32_t VulkanRHI::QueryMemoryIndex(const vk::MemoryRequirements &requirements, vk::MemoryPropertyFlags propertyFlags)
    {
        uint32_t index;

        vk::PhysicalDeviceMemoryProperties properties = physicalDevice.getMemoryProperties();
        for (uint32_t i = 0; i < properties.memoryTypeCount; i++)
        {
            if ((1 << i) & requirements.memoryTypeBits && properties.memoryTypes[i].propertyFlags & propertyFlags)
            {
                index = i;
                break;
            }
        }

        return index;
    }

    vk::Extent2D VulkanRHI::ChooseSwapchainExtent(const vk::SurfaceCapabilitiesKHR &capabilities)
    {
        if (capabilities.currentExtent != UINT32_MAX)
        {
            return capabilities.currentExtent;
        }
        else
        {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);
            vk::Extent2D extent{static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
            extent.width = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return extent;
        }
    }

    vk::RenderPass VulkanRHI::CreateRenderPass(const vk::RenderPassCreateInfo &createInfo)
    {
        // TODO:: Error check
        vk::RenderPass renderPass = device.createRenderPass(createInfo);
        return renderPass;
    }

    vk::ShaderModule VulkanRHI::CreateShaderModule(const std::vector<unsigned char> &shaderCode)
    {
        vk::ShaderModuleCreateInfo createInfo;

        createInfo.setCodeSize(shaderCode.size())
            .setPCode(reinterpret_cast<const uint32_t *>(shaderCode.data()));

        vk::ShaderModule shaderModule = device.createShaderModule(createInfo);
        return shaderModule;
    }

    vk::Pipeline VulkanRHI::CreateGraphicsPipeline(const vk::GraphicsPipelineCreateInfo &createInfo)
    {
        vk::ResultValue<vk::Pipeline> result = device.createGraphicsPipeline(nullptr, createInfo);
        if (result.result != vk::Result::eSuccess)
        {
            // TODO:: FATAL ERROR
            throw std::runtime_error("Create graphics pipeline failed!");
        }
        return result.value;
    }

    vk::PipelineLayout VulkanRHI::CreatePipelineLayout(const vk::PipelineLayoutCreateInfo &createInfo)
    {
        // TODO:: Error check
        vk::PipelineLayout layout = device.createPipelineLayout(createInfo);
        return layout;
    }

    vk::Framebuffer VulkanRHI::CreateFramebuffer(const vk::FramebufferCreateInfo &createInfo)
    {
        // TODO:: Error check
        vk::Framebuffer framebuffer = device.createFramebuffer(createInfo);
        return framebuffer;
    }

    vk::Buffer VulkanRHI::CreateBuffer(const vk::BufferCreateInfo &createInfo, vk::MemoryPropertyFlags propertyFlags, vk::DeviceMemory &deviceMemory)
    {
        vk::Buffer buffer = device.createBuffer(createInfo);

        vk::MemoryRequirements requirements = device.getBufferMemoryRequirements(buffer);

        vk::MemoryAllocateInfo allocateInfo;
        allocateInfo.setAllocationSize(requirements.size)
            .setMemoryTypeIndex(QueryMemoryIndex(requirements, propertyFlags));

        deviceMemory = device.allocateMemory(allocateInfo);

        device.bindBufferMemory(buffer, deviceMemory, 0);

        return buffer;
    }

    vk::Image VulkanRHI::CreateImage(const vk::ImageCreateInfo &createInfo, vk::MemoryPropertyFlags propertyFlags, vk::DeviceMemory &deviceMemory, vk::ImageView &imageView)
    {
        vk::Image image = device.createImage(createInfo);

        vk::MemoryRequirements requirements = device.getImageMemoryRequirements(image);
        vk::MemoryAllocateInfo allocateInfo;
        allocateInfo.setAllocationSize(requirements.size)
            .setMemoryTypeIndex(QueryMemoryIndex(requirements, propertyFlags));
        deviceMemory = device.allocateMemory(allocateInfo);
        device.bindImageMemory(image, deviceMemory, 0);

        vk::ImageSubresourceRange range;
        range.setAspectMask(vk::ImageAspectFlagBits::eColor)
            .setBaseArrayLayer(0)
            .setLayerCount(1)
            .setLevelCount(1)
            .setBaseMipLevel(0);
        vk::ImageViewCreateInfo imageViewCreateInfo;
        imageViewCreateInfo.setImage(image)
            .setViewType(vk::ImageViewType::e2D)
            .setFormat(createInfo.format)
            .setSubresourceRange(range);
        imageView = device.createImageView(imageViewCreateInfo);

        return image;
    }

    vk::ImageView VulkanRHI::CreateImageView(const vk::ImageViewCreateInfo &createInfo)
    {
        vk::ImageView imageView = device.createImageView(createInfo);
        return imageView;
    }

    vk::Sampler VulkanRHI::CreateSampler(const vk::SamplerCreateInfo &createInfo)
    {
        vk::Sampler sampler = device.createSampler(createInfo);
        return sampler;
    }

    vk::Sampler VulkanRHI::CreateDefaultLinearSampler()
    {
        vk::PhysicalDeviceProperties physicalDeviceProperties;
        physicalDeviceProperties = physicalDevice.getProperties();

        vk::SamplerCreateInfo createInfo;
        createInfo.setMagFilter(vk::Filter::eLinear)
            .setMinFilter(vk::Filter::eLinear)
            .setMipmapMode(vk::SamplerMipmapMode::eNearest)
            .setAddressModeU(vk::SamplerAddressMode::eRepeat)
            .setAddressModeV(vk::SamplerAddressMode::eRepeat)
            .setAddressModeW(vk::SamplerAddressMode::eRepeat)
            .setAnisotropyEnable(false)
            .setBorderColor(vk::BorderColor::eFloatOpaqueBlack)
            .setCompareEnable(false)
            .setUnnormalizedCoordinates(false)
            .setMaxAnisotropy(physicalDeviceProperties.limits.maxSamplerAnisotropy);

        vk::Sampler sampler = device.createSampler(createInfo);
        return sampler;
    }

    vk::DescriptorSetLayout VulkanRHI::CreateDescriptorSetLayout(const vk::DescriptorSetLayoutCreateInfo &createInfo)
    {
        vk::DescriptorSetLayout descriptorSetLayout = device.createDescriptorSetLayout(createInfo);
        return descriptorSetLayout;
    }

    vk::DescriptorSet VulkanRHI::AllocateDescriptorSets(const vk::DescriptorSetAllocateInfo &allocateInfo)
    {
        std::vector<vk::DescriptorSet> descriptorSets = device.allocateDescriptorSets(allocateInfo);
        return descriptorSets[0];
    }

    VulkanRHISwapchainDescription VulkanRHI::GetSwapchainInfo()
    {
        VulkanRHISwapchainDescription description;
        description.extent = extent;
        description.format = format;
        description.viewport = viewport;
        description.scissor = scissor;
        return description;
    }

    const std::vector<vk::ImageView>& VulkanRHI::GetSwapchainImageViews()
    {
        return swapchainImageViews;
    }

    uint32_t VulkanRHI::GetCurrentSwapchainImageIndex()
    {
        return currentSwapchainImageIndex;
    }

    const vk::CommandBuffer& VulkanRHI::GetCurrentCommandBuffer()
    {
        return commandBuffers[currentFrame];
    }

    const vk::DescriptorPool& VulkanRHI::GetDescriptorPool()
    {
        return descriptorPool;
    }

    vk::CommandBuffer VulkanRHI::BeginOneTimeCommandBuffer()
    {
        vk::CommandBufferAllocateInfo allocateInfo;

        allocateInfo.setCommandBufferCount(1)
            .setLevel(vk::CommandBufferLevel::ePrimary)
            .setCommandPool(commandPool);

        vk::CommandBuffer commandBuffer = device.allocateCommandBuffers(allocateInfo)[0];

        vk::CommandBufferBeginInfo beginInfo;
        beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        commandBuffer.begin(beginInfo);

        return commandBuffer;
    }

    void VulkanRHI::EndOneTimeCommandBuffer(const vk::CommandBuffer &commandBuffer)
    {
        commandBuffer.end();

        vk::SubmitInfo submitInfo;
        submitInfo.setCommandBuffers(commandBuffer);
        graphicsQueue.submit(submitInfo);
        graphicsQueue.waitIdle();

        device.freeCommandBuffers(commandPool, commandBuffer);
    }
    
    void* VulkanRHI::MapMemory(const vk::DeviceMemory &deviceMemory, vk::DeviceSize offset, vk::DeviceSize size)
    {
        void* ptr = device.mapMemory(deviceMemory, offset, size);
        return ptr;
    }

    void VulkanRHI::UnmapMemory(const vk::DeviceMemory &deviceMemory)
    {
        device.unmapMemory(deviceMemory);
    }

    void VulkanRHI::FreeMemory(const vk::DeviceMemory &deviceMemory)
    {
        device.freeMemory(deviceMemory);
    }
    
    void VulkanRHI::UpdateDescriptorSets(const std::vector<vk::WriteDescriptorSet> &writes, const std::vector<vk::CopyDescriptorSet> &copies)
    {
        device.updateDescriptorSets(writes, copies);
    }

    void VulkanRHI::PrepareBeforeRender()
    {
        WaitForFences();
        ResetCommandPool();
        AcquireNextImage();
        
        vk::CommandBufferBeginInfo beginInfo;
        commandBuffers[currentFrame].begin(beginInfo);
    }

    void VulkanRHI::WaitForFences()
    {
        vk::Result result = device.waitForFences(inFlightFences[currentFrame], true, std::numeric_limits<uint64_t>::max());
        if (result != vk::Result::eSuccess)
        {
            // TODO:: change it to LOG_ERROR or std<<cout
            throw std::runtime_error("Failed to synchronize!");
        }
    }
    void VulkanRHI::AcquireNextImage()
    {
        vk::ResultValue<uint32_t> result = device.acquireNextImageKHR(swapchain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphores[currentFrame]);
        if (result.result != vk::Result::eSuccess)
        {
            // TODO:: change it to LOG_ERROR or std<<cout
            throw std::runtime_error("Acquire next image failed!");
        }
        currentSwapchainImageIndex = result.value;
    }

    void VulkanRHI::ResetCommandPool()
    {
        device.resetCommandPool(commandPools[currentFrame]);
    }

    void VulkanRHI::SubmitRendering()
    {
        commandBuffers[currentFrame].end();

        device.resetFences(inFlightFences[currentFrame]);

        std::vector<vk::PipelineStageFlags> waitStages{vk::PipelineStageFlagBits::eColorAttachmentOutput};

        vk::SubmitInfo submitInfo;
        submitInfo.setCommandBuffers(commandBuffers[currentFrame])
            .setWaitSemaphores(imageAvailableSemaphores[currentFrame])
            .setWaitDstStageMask(waitStages)
            .setSignalSemaphores(renderFinishedSemaphores[currentFrame]);
        graphicsQueue.submit(submitInfo, inFlightFences[currentFrame]);

        vk::PresentInfoKHR presentInfo;
        presentInfo.setSwapchains(swapchain)
            .setImageIndices(currentSwapchainImageIndex)
            .setWaitSemaphores(renderFinishedSemaphores[currentFrame]);
        vk::Result result = presentQueue.presentKHR(presentInfo);
        if (result != vk::Result::eSuccess)
        {
            throw std::runtime_error("Failed to present image!");
        }

        currentFrame = (currentFrame + 1) % maxFramesInFlight;
    }

    void VulkanRHI::CommandBindPipeline(const vk::CommandBuffer &commandBuffer, vk::PipelineBindPoint bindPoint, const vk::Pipeline &pipeline)
    {
        commandBuffer.bindPipeline(bindPoint, pipeline);
    }
    
    void VulkanRHI::CommandBindDescriptorSet(const vk::CommandBuffer &commandBuffer, vk::PipelineBindPoint bindPoint, const vk::PipelineLayout &layout, uint32_t firstSet, const std::vector<vk::DescriptorSet> &descriptorSets, std::vector<uint32_t> dynamicOffsets)
    {
        commandBuffer.bindDescriptorSets(bindPoint, layout, firstSet, descriptorSets, dynamicOffsets);
    }

    void VulkanRHI::CommandBindVertexBuffers(const vk::CommandBuffer &commandBuffer, uint32_t firstBinding, const std::vector<vk::Buffer> &vertexBuffers, std::vector<vk::DeviceSize> offsets)
    {
        commandBuffer.bindVertexBuffers(firstBinding, vertexBuffers, offsets);
    }

    void VulkanRHI::CommandBindIndexBuffer(const vk::CommandBuffer &commandBuffer, const vk::Buffer &indexBuffer, vk::DeviceSize offset, vk::IndexType indexType)
    {
        commandBuffer.bindIndexBuffer(indexBuffer, offset, indexType);
    }

    void VulkanRHI::CommandBeginRenderPass(const vk::CommandBuffer &commandBuffer, const vk::RenderPassBeginInfo &beginInfo, vk::SubpassContents subpassContents)
    {
        commandBuffer.beginRenderPass(beginInfo, subpassContents);
    }

    void VulkanRHI::CommandEndRenderPass(const vk::CommandBuffer &commandBuffer)
    {
        commandBuffer.endRenderPass();
    }

    void VulkanRHI::CommandDraw(const vk::CommandBuffer &commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
    {
        commandBuffer.draw(vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void VulkanRHI::CommandDrawIndexed(const vk::CommandBuffer &commandBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance)
    {
        commandBuffer.drawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }

    void VulkanRHI::CommandCopyBuffer(const vk::CommandBuffer &commandBuffer, const vk::Buffer &srcBuffer, const vk::Buffer &dstBuffer, const vk::BufferCopy &region)
    {
        commandBuffer.copyBuffer(srcBuffer, dstBuffer, region);
    }

    void VulkanRHI::CommandCopyBufferToImage(const vk::CommandBuffer &commandBuffer, const vk::Buffer &srcBuffer, const vk::Image &dstImage, vk::ImageLayout dstImageLayout, const vk::BufferImageCopy &region)
    {
        commandBuffer.copyBufferToImage(srcBuffer, dstImage, dstImageLayout, region);
    }

    void VulkanRHI::CommandPipelineBarrier(const vk::CommandBuffer &commandBuffer, vk::PipelineStageFlags srcStageMask, vk::PipelineStageFlags dstStageMask, vk::DependencyFlags dependencyFlags, const std::vector<vk::MemoryBarrier> &memoryBarriers, const std::vector<vk::BufferMemoryBarrier> &bufferMemoryBarriers, const std::vector<vk::ImageMemoryBarrier> &imageMemoryBarriers)
    {
        commandBuffer.pipelineBarrier(srcStageMask, dstStageMask, dependencyFlags, memoryBarriers, bufferMemoryBarriers, imageMemoryBarriers);
    }

    void VulkanRHI::WaitIdle()
    {
        device.waitIdle();
    }

    void VulkanRHI::DestroyRenderPass(const vk::RenderPass &renderPass)
    {
        device.destroyRenderPass(renderPass);
    }

    void VulkanRHI::DestroyPipeline(const vk::Pipeline &pipeline)
    {
        device.destroyPipeline(pipeline);
    }

    void VulkanRHI::DestroyPipelineLayout(const vk::PipelineLayout &layout)
    {
        device.destroyPipelineLayout(layout);
    }

    void VulkanRHI::DestroyShaderModule(const vk::ShaderModule &shaderModule)
    {
        device.destroyShaderModule(shaderModule);
    }

    void VulkanRHI::DestroyFramebuffer(const vk::Framebuffer &framebuffer)
    {
        device.destroyFramebuffer(framebuffer);
    }

    void VulkanRHI::DestroyBuffer(const vk::Buffer &buffer)
    {
        device.destroyBuffer(buffer);
    }

    void VulkanRHI::DestroyImage(const vk::Image &image)
    {
        device.destroyImage(image);
    }

    void VulkanRHI::DestroyImageView(const vk::ImageView &imageView)
    {
        device.destroyImageView(imageView);
    }

    void VulkanRHI::DestroySampler(const vk::Sampler &sampler)
    {
        device.destroySampler(sampler);
    }

    void VulkanRHI::DestroyDescriptorSetLayout(const vk::DescriptorSetLayout &descriptorSetLayout)
    {
        device.destroyDescriptorSetLayout(descriptorSetLayout);
    }
}