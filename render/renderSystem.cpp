#include "render/renderSystem.hpp"

#include <cstdint>

namespace XrayEngine
{
    const std::vector<MeshVertex> meshData{
        {glm::vec2{-0.5, 0.5}, glm::vec2{0.0, 1.0}},
        {glm::vec2{0.5, 0.5}, glm::vec2{1.0, 1.0}},
        {glm::vec2{0.5, -0.5}, glm::vec2{1.0, 0.0}},
        {glm::vec2{-0.5, -0.5}, glm::vec2{0.0, 0.0}},
    };

    const std::vector<uint16_t> indices{
        0, 1, 3,
        1, 2, 3,
    };

    RenderSystem::~RenderSystem() {}

    void RenderSystem::Initialize(const RenderSystemInitInfo &renderSystemInitInfo)
    {
        inputSystem = renderSystemInitInfo.inputSystem;

        VulkanRHIInitInfo vulkanRHIInitInfo;
        vulkanRHIInitInfo.window = renderSystemInitInfo.window;
        vulkanRHIInitInfo.windowWidth = renderSystemInitInfo.windowWidth;
        vulkanRHIInitInfo.windowHeight = renderSystemInitInfo.windowHeight;
        vulkanRHI = std::make_shared<VulkanRHI>();
        vulkanRHI->Initialize(vulkanRHIInitInfo);

        RenderCameraInitInfo renderCameraInitInfo;
        renderCameraInitInfo.inputSystem = inputSystem;
        renderCamera = std::make_shared<RenderCamera>();
        renderCamera->Initialize(renderCameraInitInfo);

        RenderResourceInitInfo renderResourceInitInfo;
        renderResourceInitInfo.vulkanRHI = vulkanRHI;
        renderResource = std::make_shared<RenderResource>();
        renderResource->Initialize(renderResourceInitInfo);
        TextureData textureData = renderResource->LoadTexture("asset/texture.png");
        renderResource->CreateMeshes(meshData);
        renderResource->CreateTextures(textureData);

        MainCameraPassInitInfo mainCameraPassInitInfo;
        mainCameraPassInitInfo.vulkanRHI = vulkanRHI;
        mainCameraPassInitInfo.renderResource = renderResource;
        mainCameraPass = std::make_shared<MainCameraPass>();
        mainCameraPass->Initialize(&mainCameraPassInitInfo);

        renderResource->CreateIndices(indices);
    }
    
    void RenderSystem::Quit()
    {
        vulkanRHI->WaitIdle();

        renderResource->Quit();
        mainCameraPass->Quit();
        vulkanRHI->Quit();
    }

    void RenderSystem::RenderTick()
    {
        renderCamera->Update();

        renderResource->UpdatePerFrameBuffer(renderCamera);

        vulkanRHI->PrepareBeforeRender();

        mainCameraPass->Draw();

        vulkanRHI->SubmitRendering();
    }
}