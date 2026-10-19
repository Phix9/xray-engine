#include "render/renderSystem.hpp"

namespace XrayEngine
{
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

        renderScene = std::make_shared<RenderScene>();
        renderScene->ambientLight = {glm::vec3{0.03f}};

        renderScene->directionLight = {
            glm::normalize(glm::vec3{-0.5f, -1.0f, -0.3f}),
            glm::vec3{1.0f, 0.96f, 0.9f}};

        PointLight warmLight;
        warmLight.position = glm::vec3{2.0f, 2.0f, 2.0f};
        warmLight.intensity = glm::vec3{20.0f, 15.0f, 10.0f};
        renderScene->pointLightList.lights.push_back(warmLight);

        PointLight coolLight;
        coolLight.position = glm::vec3{-2.0f, 1.5f, 2.0f};
        coolLight.intensity = glm::vec3{8.0f, 12.0f, 20.0f};
        renderScene->pointLightList.lights.push_back(coolLight);

        PointLight backLight;
        backLight.position = glm::vec3{0.0f, 2.5f, -2.5f};
        backLight.intensity = glm::vec3{15.0f, 15.0f, 15.0f};
        renderScene->pointLightList.lights.push_back(backLight);

        RenderCameraInitInfo renderCameraInitInfo;
        renderCameraInitInfo.inputSystem = inputSystem;
        renderCamera = std::make_shared<RenderCamera>();
        renderCamera->Initialize(renderCameraInitInfo);

        RenderResourceInitInfo renderResourceInitInfo;
        renderResourceInitInfo.vulkanRHI = vulkanRHI;
        renderResource = std::make_shared<RenderResource>();
        renderResource->Initialize(renderResourceInitInfo);
        MeshData meshData = renderResource->LoadMesh("asset/models/violin/violin.obj");

        Material material;
        material.baseColorTexture = "asset/models/violin/BaseColor.png";
        material.metallicTexture = "asset/models/violin/Metallic.png";
        material.roughnessTexture = "asset/models/violin/Roughness.png";
        material.normalTexture = "asset/models/violin/Normal.png";
        material.AOTexture = "asset/models/violin/AO.png";
        RenderMaterialData materialData = renderResource->LoadMaterial(material);

        renderResource->CreateMeshes(meshData.vertices);
        renderResource->CreateMaterial(materialData);
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

        renderResource->UpdatePerFramebuffer(renderScene, renderCamera);

        vulkanRHI->PrepareBeforeRender();

        mainCameraPass->Draw();

        vulkanRHI->SubmitRendering();
    }
}