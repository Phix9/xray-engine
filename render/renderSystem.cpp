#include "render/renderSystem.hpp"

namespace XrayEngine
{
    // const std::vector<MeshVertex> meshData{
    //     // First cube
    //     // +Z (front)
    //     {glm::vec3{-0.5f, 0.5f, 0.5f}, glm::vec2{0.0f, 1.0f}},
    //     {glm::vec3{-0.5f, -0.5f, 0.5f}, glm::vec2{0.0f, 0.0f}},
    //     {glm::vec3{0.5f, -0.5f, 0.5f}, glm::vec2{1.0f, 0.0f}},
    //     {glm::vec3{0.5f, 0.5f, 0.5f}, glm::vec2{1.0f, 1.0f}},
    //     // -Z (back)
    //     {glm::vec3{0.5f, 0.5f, -0.5f}, glm::vec2{0.0f, 1.0f}},
    //     {glm::vec3{0.5f, -0.5f, -0.5f}, glm::vec2{0.0f, 0.0f}},
    //     {glm::vec3{-0.5f, -0.5f, -0.5f}, glm::vec2{1.0f, 0.0f}},
    //     {glm::vec3{-0.5f, 0.5f, -0.5f}, glm::vec2{1.0f, 1.0f}},
    //     // +X (right)
    //     {glm::vec3{0.5f, 0.5f, 0.5f}, glm::vec2{0.0f, 1.0f}},
    //     {glm::vec3{0.5f, -0.5f, 0.5f}, glm::vec2{0.0f, 0.0f}},
    //     {glm::vec3{0.5f, -0.5f, -0.5f}, glm::vec2{1.0f, 0.0f}},
    //     {glm::vec3{0.5f, 0.5f, -0.5f}, glm::vec2{1.0f, 1.0f}},
    //     // -X (left)
    //     {glm::vec3{-0.5f, 0.5f, -0.5f}, glm::vec2{0.0f, 1.0f}},
    //     {glm::vec3{-0.5f, -0.5f, -0.5f}, glm::vec2{0.0f, 0.0f}},
    //     {glm::vec3{-0.5f, -0.5f, 0.5f}, glm::vec2{1.0f, 0.0f}},
    //     {glm::vec3{-0.5f, 0.5f, 0.5f}, glm::vec2{1.0f, 1.0f}},
    //     // +Y (top)
    //     {glm::vec3{0.5f, 0.5f, -0.5f}, glm::vec2{0.0f, 1.0f}},
    //     {glm::vec3{-0.5f, 0.5f, -0.5f}, glm::vec2{0.0f, 0.0f}},
    //     {glm::vec3{-0.5f, 0.5f, 0.5f}, glm::vec2{1.0f, 0.0f}},
    //     {glm::vec3{0.5f, 0.5f, 0.5f}, glm::vec2{1.0f, 1.0f}},
    //     // -Y (bottom)
    //     {glm::vec3{-0.5f, -0.5f, -0.5f}, glm::vec2{0.0f, 1.0f}},
    //     {glm::vec3{0.5f, -0.5f, -0.5f}, glm::vec2{0.0f, 0.0f}},
    //     {glm::vec3{0.5f, -0.5f, 0.5f}, glm::vec2{1.0f, 0.0f}},
    //     {glm::vec3{-0.5f, -0.5f, 0.5f}, glm::vec2{1.0f, 1.0f}},

    //     // Second cube
    //     // +Z (front)
    //     {glm::vec3{0.2f, 2.0f, 1.2f}, glm::vec2{0.0f, 1.0f}},
    //     {glm::vec3{0.2f, 1.0f, 1.2f}, glm::vec2{0.0f, 0.0f}},
    //     {glm::vec3{1.2f, 1.0f, 1.2f}, glm::vec2{1.0f, 0.0f}},
    //     {glm::vec3{1.2f, 2.0f, 1.2f}, glm::vec2{1.0f, 1.0f}},
    //     // -Z (back)
    //     {glm::vec3{1.2f, 2.0f, 0.2f}, glm::vec2{0.0f, 1.0f}},
    //     {glm::vec3{1.2f, 1.0f, 0.2f}, glm::vec2{0.0f, 0.0f}},
    //     {glm::vec3{0.2f, 1.0f, 0.2f}, glm::vec2{1.0f, 0.0f}},
    //     {glm::vec3{0.2f, 2.0f, 0.2f}, glm::vec2{1.0f, 1.0f}},
    //     // +X (right)
    //     {glm::vec3{1.2f, 2.0f, 1.2f}, glm::vec2{0.0f, 1.0f}},
    //     {glm::vec3{1.2f, 1.0f, 1.2f}, glm::vec2{0.0f, 0.0f}},
    //     {glm::vec3{1.2f, 1.0f, 0.2f}, glm::vec2{1.0f, 0.0f}},
    //     {glm::vec3{1.2f, 2.0f, 0.2f}, glm::vec2{1.0f, 1.0f}},
    //     // -X (left)
    //     {glm::vec3{0.2f, 2.0f, 0.2f}, glm::vec2{0.0f, 1.0f}},
    //     {glm::vec3{0.2f, 1.0f, 0.2f}, glm::vec2{0.0f, 0.0f}},
    //     {glm::vec3{0.2f, 1.0f, 1.2f}, glm::vec2{1.0f, 0.0f}},
    //     {glm::vec3{0.2f, 2.0f, 1.2f}, glm::vec2{1.0f, 1.0f}},
    //     // +Y (top)
    //     {glm::vec3{1.2f, 2.0f, 0.2f}, glm::vec2{0.0f, 1.0f}},
    //     {glm::vec3{0.2f, 2.0f, 0.2f}, glm::vec2{0.0f, 0.0f}},
    //     {glm::vec3{0.2f, 2.0f, 1.2f}, glm::vec2{1.0f, 0.0f}},
    //     {glm::vec3{1.2f, 2.0f, 1.2f}, glm::vec2{1.0f, 1.0f}},
    //     // -Y (bottom)
    //     {glm::vec3{0.2f, 1.0f, 0.2f}, glm::vec2{0.0f, 1.0f}},
    //     {glm::vec3{1.2f, 1.0f, 0.2f}, glm::vec2{0.0f, 0.0f}},
    //     {glm::vec3{1.2f, 1.0f, 1.2f}, glm::vec2{1.0f, 0.0f}},
    //     {glm::vec3{0.2f, 1.0f, 1.2f}, glm::vec2{1.0f, 1.0f}},
    // };

    // const std::vector<uint16_t> indices{
    //     // First cube (vertices 0..23)
    //     0, 1, 3, 1, 2, 3,
    //     4, 5, 7, 5, 6, 7,
    //     8, 9, 11, 9, 10, 11,
    //     12, 13, 15, 13, 14, 15,
    //     16, 17, 19, 17, 18, 19,
    //     20, 21, 23, 21, 22, 23,
    //     // Second cube (vertices 24..47)
    //     24, 25, 27, 25, 26, 27,
    //     28, 29, 31, 29, 30, 31,
    //     32, 33, 35, 33, 34, 35,
    //     36, 37, 39, 37, 38, 39,
    //     40, 41, 43, 41, 42, 43,
    //     44, 45, 47, 45, 46, 47,
    // };

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
        MeshData meshData = renderResource->LoadMesh("asset/models/viking_room.obj");
        TextureData textureData = renderResource->LoadTexture("asset/models/viking_room.png");
        renderResource->CreateMeshes(meshData.vertices);
        renderResource->CreateTextures(textureData);
        renderResource->CreateStorageBuffer();

        MainCameraPassInitInfo mainCameraPassInitInfo;
        mainCameraPassInitInfo.vulkanRHI = vulkanRHI;
        mainCameraPassInitInfo.renderResource = renderResource;
        mainCameraPass = std::make_shared<MainCameraPass>();
        mainCameraPass->Initialize(&mainCameraPassInitInfo);

        renderResource->CreateIndices(meshData.indices);
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