#pragma once

#include <vulkan/vulkan.hpp>

#include "render/interface/vulkanRHI.hpp"

namespace XrayEngine
{
    struct RenderPassInitInfo
    {
        std::shared_ptr<VulkanRHI> vulkanRHI;
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
    };
}