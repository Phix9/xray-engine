#pragma once

#include <vulkan/vulkan.hpp>

#include "render/interface/vulkanRHI.hpp"
#include "render/renderResource.hpp"

namespace XrayEngine
{
    struct RenderPassInitInfo
    {
        std::shared_ptr<VulkanRHI> vulkanRHI;
        std::shared_ptr<RenderResource> renderResource;
    };

    struct FramebufferAttachment
    {
        vk::Image image;
        vk::DeviceMemory memory;
        vk::ImageView imageView;
        vk::Format format;
    };

    class RenderPass
    {
    public:
        struct RenderPipeline
        {
            vk::PipelineLayout layout;
            vk::Pipeline pipeline;
        };

        virtual void Initialize(const RenderPassInitInfo* initInfo);

    protected:
        std::shared_ptr<VulkanRHI> vulkanRHI;
        std::shared_ptr<RenderResource> renderResource;

        vk::RenderPass renderPass;
        vk::Framebuffer framebuffer;
        std::vector<FramebufferAttachment> framebufferAttachments;
    };
}