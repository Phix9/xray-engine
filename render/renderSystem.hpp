#pragma once

#include <iostream>
#include <memory>
#include <vulkan/vulkan.hpp>

#include "input/inputSystem.hpp"
#include "render/interface/vulkanRHI.hpp"
#include "render/renderResource.hpp"
#include "render/renderCamera.hpp"
#include "render/renderPass.hpp"
#include "render/passes/mainCameraPass.hpp"

namespace XrayEngine
{
    struct RenderSystemInitInfo
    {
        std::shared_ptr<InputSystem> inputSystem;
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

        void RenderTick();

    private:
        std::shared_ptr<InputSystem> inputSystem;

        std::shared_ptr<VulkanRHI> vulkanRHI;
        std::shared_ptr<RenderCamera> renderCamera;
        std::shared_ptr<RenderResource> renderResource;
        std::shared_ptr<MainCameraPass> mainCameraPass;
    };
}