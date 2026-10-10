#include "render/renderSystem.hpp"

#include <cstdint>

namespace XrayEngine
{
    // Corner order per face is TL, BL, BR, TR of that face. With the index pattern
    // below, that makes the right-hand-rule normal of every face equal to its outward
    // normal, which is what back-face culling needs. Only the -Y face already had
    // this order; the other five were wound the other way round.
    const std::vector<MeshVertex> meshData{
        // +Z (front)
        {glm::vec3{-0.5f, 0.5f, 0.5f}, glm::vec2{0.0f, 1.0f}},
        {glm::vec3{-0.5f, -0.5f, 0.5f}, glm::vec2{0.0f, 0.0f}},
        {glm::vec3{0.5f, -0.5f, 0.5f}, glm::vec2{1.0f, 0.0f}},
        {glm::vec3{0.5f, 0.5f, 0.5f}, glm::vec2{1.0f, 1.0f}},
        // -Z (back)
        {glm::vec3{0.5f, 0.5f, -0.5f}, glm::vec2{0.0f, 1.0f}},
        {glm::vec3{0.5f, -0.5f, -0.5f}, glm::vec2{0.0f, 0.0f}},
        {glm::vec3{-0.5f, -0.5f, -0.5f}, glm::vec2{1.0f, 0.0f}},
        {glm::vec3{-0.5f, 0.5f, -0.5f}, glm::vec2{1.0f, 1.0f}},
        // +X (right)
        {glm::vec3{0.5f, 0.5f, 0.5f}, glm::vec2{0.0f, 1.0f}},
        {glm::vec3{0.5f, -0.5f, 0.5f}, glm::vec2{0.0f, 0.0f}},
        {glm::vec3{0.5f, -0.5f, -0.5f}, glm::vec2{1.0f, 0.0f}},
        {glm::vec3{0.5f, 0.5f, -0.5f}, glm::vec2{1.0f, 1.0f}},
        // -X (left)
        {glm::vec3{-0.5f, 0.5f, -0.5f}, glm::vec2{0.0f, 1.0f}},
        {glm::vec3{-0.5f, -0.5f, -0.5f}, glm::vec2{0.0f, 0.0f}},
        {glm::vec3{-0.5f, -0.5f, 0.5f}, glm::vec2{1.0f, 0.0f}},
        {glm::vec3{-0.5f, 0.5f, 0.5f}, glm::vec2{1.0f, 1.0f}},
        // +Y (top)
        {glm::vec3{0.5f, 0.5f, -0.5f}, glm::vec2{0.0f, 1.0f}},
        {glm::vec3{-0.5f, 0.5f, -0.5f}, glm::vec2{0.0f, 0.0f}},
        {glm::vec3{-0.5f, 0.5f, 0.5f}, glm::vec2{1.0f, 0.0f}},
        {glm::vec3{0.5f, 0.5f, 0.5f}, glm::vec2{1.0f, 1.0f}},
        // -Y (bottom)
        {glm::vec3{-0.5f, -0.5f, -0.5f}, glm::vec2{0.0f, 1.0f}},
        {glm::vec3{0.5f, -0.5f, -0.5f}, glm::vec2{0.0f, 0.0f}},
        {glm::vec3{0.5f, -0.5f, 0.5f}, glm::vec2{1.0f, 0.0f}},
        {glm::vec3{-0.5f, -0.5f, 0.5f}, glm::vec2{1.0f, 1.0f}},
    };

    const std::vector<uint16_t> indices{
        0, 1, 3, 1, 2, 3,
        4, 5, 7, 5, 6, 7,
        8, 9, 11, 9, 10, 11,
        12, 13, 15, 13, 14, 15,
        16, 17, 19, 17, 18, 19,
        20, 21, 23, 21, 22, 23,
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
        renderResource->CreateStorageBuffer();

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

        renderResource->ResetRingBufferOffset();

        renderResource->UpdatePerFrameBuffer(renderCamera);

        vulkanRHI->PrepareBeforeRender();

        mainCameraPass->Draw();

        vulkanRHI->SubmitRendering();
    }
}