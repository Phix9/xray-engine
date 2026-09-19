#pragma once

#include <vulkan/vulkan.hpp>

namespace XrayEngine
{
    struct VulkanRHISwapchainDescription
    {
        vk::Extent2D extent;
        vk::Format format;
        vk::Viewport viewport;
        vk::Rect2D scissor;
    };
}