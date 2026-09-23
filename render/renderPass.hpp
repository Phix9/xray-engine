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
    };
}