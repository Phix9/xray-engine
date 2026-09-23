#pragma once

#include <algorithm>
#include <set>
#include <memory>
#include <optional>
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include "render/interface/vulkanRHIStruct.hpp"

namespace XrayEngine
{
    struct VulkanRHIInitInfo
    {
        GLFWwindow *window;
        float windowWidth;
        float windowHeight;
    };

    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;
        std::optional<uint32_t> computeFamily;

        bool isComplete()
        {
            return graphicsFamily.has_value() && presentFamily.has_value() && computeFamily.has_value();
        }
    };

    struct SwapchainSupportDetails
    {
        vk::SurfaceCapabilitiesKHR capabilities;
        vk::SurfaceFormatKHR surfaceFormat;
        vk::PresentModeKHR presentMode;
    };

    class VulkanRHI
    {
    public:
        void Initialize(const VulkanRHIInitInfo& initInfo);
        void Quit();

        vk::Pipeline CreateGraphicsPipelines(const vk::GraphicsPipelineCreateInfo &CreateInfo);
        vk::RenderPass CreateRenderPass(const vk::RenderPassCreateInfo &createInfo);
        vk::ShaderModule CreateShaderModule(const std::vector<unsigned char> &shaderCode);
        vk::Pipeline CreateGraphicsPipeline(const vk::GraphicsPipelineCreateInfo &createInfo);
        vk::PipelineLayout CreatePipelineLayout(const vk::PipelineLayoutCreateInfo &createInfo);
        vk::Framebuffer CreateFramebuffer(const vk::FramebufferCreateInfo &createInfo);
        vk::Buffer CreateBuffer(const vk::BufferCreateInfo &createInfo, vk::MemoryPropertyFlags propertyFlags, vk::DeviceMemory &deviceMemory);
        vk::Image CreateImage(const vk::ImageCreateInfo &createInfo, vk::MemoryPropertyFlags propertyFlags, vk::DeviceMemory &deviceMemory, vk::ImageView &imageView);
        vk::ImageView CreateImageView(const vk::ImageViewCreateInfo &createInfo);
        vk::Sampler CreateSampler(const vk::SamplerCreateInfo &createInfo);
        vk::Sampler CreateDefaultLinearSampler();
        vk::DescriptorSetLayout CreateDescriptorSetLayout(const vk::DescriptorSetLayoutCreateInfo &createInfo);
        vk::DescriptorSet AllocateDescriptorSets(const vk::DescriptorSetAllocateInfo &allocateInfo);
        VulkanRHISwapchainDescription GetSwapchainInfo();
        const std::vector<vk::ImageView>& GetSwapchainImageViews();
        uint32_t GetCurrentSwapchainImageIndex();
        const vk::CommandBuffer& GetCurrentCommandBuffer();
        const vk::DescriptorPool& GetDescriptorPool();
        vk::CommandBuffer BeginOneTimeCommandBuffer();
        void EndOneTimeCommandBuffer(const vk::CommandBuffer &commandBuffer);
        void* MapMemory(const vk::DeviceMemory &deviceMemory, vk::DeviceSize offset, vk::DeviceSize size);
        void UnmapMemory(const vk::DeviceMemory &deviceMemory);
        void FreeMemory(const vk::DeviceMemory &deviceMemory);
        void UpdateDescriptorSets(const std::vector<vk::WriteDescriptorSet> &writes, const std::vector<vk::CopyDescriptorSet> &copies);
        void PrepareBeforeRender();
        void SubmitRendering();
        void CommandBindPipeline(const vk::CommandBuffer &commandBuffer, vk::PipelineBindPoint bindPoint, const vk::Pipeline &pipeline);
        void CommandBindDescriptorSet(const vk::CommandBuffer &commandBUffer, vk::PipelineBindPoint bindPoint, const vk::PipelineLayout &layout, uint32_t firstSet, const std::vector<vk::DescriptorSet> &descriptorSets, std::vector<uint32_t> dynamicOffsets);
        void CommandBindVertexBuffers(const vk::CommandBuffer &commandBuffer, uint32_t firstBinding, const std::vector<vk::Buffer> &vertexBuffers, std::vector<vk::DeviceSize> offsets);
        void CommandBindIndexBuffer(const vk::CommandBuffer &commandBuffer, const vk::Buffer &indexBuffer, vk::DeviceSize offset, vk::IndexType indexType);
        void CommandBeginRenderPass(const vk::CommandBuffer &commandBuffer, const vk::RenderPassBeginInfo &beginInfo, vk::SubpassContents subpassContents);
        void CommandEndRenderPass(const vk::CommandBuffer &commandBuffer);
        void CommandDraw(const vk::CommandBuffer &commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
        void CommandDrawIndexed(const vk::CommandBuffer &commandBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance);
        void CommandCopyBuffer(const vk::CommandBuffer &commandBuffer, const vk::Buffer &srcBuffer, const vk::Buffer &dstBuffer, const vk::BufferCopy &region);
        void CommandCopyBufferToImage(const vk::CommandBuffer &commandBuffer, const vk::Buffer &srcBuffer, const vk::Image &dstImage, vk::ImageLayout dstImageLayout, const vk::BufferImageCopy &region);
        void CommandPipelineBarrier(const vk::CommandBuffer &commandBuffer, vk::PipelineStageFlags srcStageMask, vk::PipelineStageFlags dstStageMask, vk::DependencyFlags dependencyFlags,  const std::vector<vk::MemoryBarrier> &memoryBarriers, const std::vector<vk::BufferMemoryBarrier> &bufferMemoryBarriers,  const std::vector<vk::ImageMemoryBarrier> &imageMemoryBarriers);
        void WaitIdle();
        void DestroyRenderPass(const vk::RenderPass &renderPass);
        void DestroyPipeline(const vk::Pipeline &pipeline);
        void DestroyPipelineLayout(const vk::PipelineLayout &layout);
        void DestroyShaderModule(const vk::ShaderModule &shaderModule);
        void DestroyFramebuffer(const vk::Framebuffer &framebuffer);
        void DestroyBuffer(const vk::Buffer &buffer);
        void DestroyImage(const vk::Image &image);
        void DestroyImageView(const vk::ImageView &imageView);
        void DestroySampler(const vk::Sampler &sampler);
        void DestroyDescriptorSetLayout(const vk::DescriptorSetLayout &descriptorSetLayout);

        private:
        static const uint8_t maxFramesInFlight = 3;
        uint8_t currentFrame = 0;   
        uint32_t currentSwapchainImageIndex;
        
        vk::Instance instance;
        vk::PhysicalDevice physicalDevice;
        vk::Device device;
        vk::Queue graphicsQueue;
        vk::Queue presentQueue;
        vk::SurfaceKHR surface;
        vk::SwapchainKHR swapchain;
        vk::Extent2D extent;
        vk::Format format;
        vk::Viewport viewport;
        vk::Rect2D scissor;
        std::vector<vk::Image> swapchainImages;
        std::vector<vk::ImageView> swapchainImageViews;
        std::vector<vk::Semaphore> imageAvailableSemaphores{maxFramesInFlight};
        std::vector<vk::Semaphore> renderFinishedSemaphores{maxFramesInFlight};
        std::vector<vk::Fence> inFlightFences{maxFramesInFlight};
        std::vector<vk::CommandPool> commandPools{maxFramesInFlight};
        std::vector<vk::CommandBuffer> commandBuffers{maxFramesInFlight};
        vk::CommandPool commandPool;
        vk::DescriptorPool descriptorPool;
        QueueFamilyIndices queueFamilyIndices;
        GLFWwindow *window;
        
        void CreateInstance();
        void InitializePhysicalDevice();
        void CreateLogicalDevice();
        void CreateWindowSurface();
        void CreateSwapchain();
        void CreateSwapchainImageViews();
        void CreateCommandPool();
        void CreateSemaphores();
        void CreateFences();
        void CreateDescriptorPool();
        void AllocateCommandBuffers();
        void FreeCommandBuffers();
        void WaitForFences();
        void ResetCommandPool();
        void AcquireNextImage();
        std::vector<const char *> GetRequiredExtensions();
        QueueFamilyIndices QueryQueueFamilies();
        SwapchainSupportDetails QuerySwapchainSupport();
        uint32_t QueryMemoryIndex(const vk::MemoryRequirements &requirements, vk::MemoryPropertyFlags propertyFlags);
        vk::Extent2D ChooseSwapchainExtent(const vk::SurfaceCapabilitiesKHR &capabilities);
    };
}