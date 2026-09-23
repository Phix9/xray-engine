#pragma once

#include <iostream>
#include <memory>
#include <vulkan/vulkan.hpp>

#include "render/interface/vulkanRHI.hpp"
#include "render/renderResource.hpp"
#include "render/renderPass.hpp"
#include "render/passes/mainCameraPass.hpp"

namespace XrayEngine
{
    struct RenderSystemInitInfo
    {
        GLFWwindow *window;
        float windowWidth;
        float windowHeight;
    };

    class RenderSystem
    {
    public:
        RenderSystem() = default;
        ~RenderSystem();

        void Initialize(const RenderSystemInitInfo& renderSystemInitInfo);
        void Quit();

        void Render();

    private:
        void UpdateUniformBuffer();

        vk::Buffer uniformBuffer;
        vk::DeviceMemory uniformBufferMemory;
        void *mappedUniformBuffer{nullptr};

        std::shared_ptr<VulkanRHI> vulkanRHI;
        std::shared_ptr<RenderResource> renderResource;
        std::shared_ptr<MainCameraPass> mainCameraPass;
    };
}