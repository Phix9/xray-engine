#include "render/renderSystem.hpp"

#include <cmath>
#include <cstring>
#include <glm/gtc/constants.hpp>

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

    glm::vec3 color{0.0, 1.0, 0.0};

    RenderSystem::~RenderSystem() {}

    void RenderSystem::Initialize(const RenderSystemInitInfo &renderSystemInitInfo)
    {
        VulkanRHIInitInfo vulkanRHIInitInfo;
        vulkanRHIInitInfo.window = renderSystemInitInfo.window;
        vulkanRHIInitInfo.windowWidth = renderSystemInitInfo.windowWidth;
        vulkanRHIInitInfo.windowHeight = renderSystemInitInfo.windowHeight;
        vulkanRHI = std::make_shared<VulkanRHI>();
        vulkanRHI->Initialize(vulkanRHIInitInfo);

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

        // initialize uniform buffer
        {
            const vk::DeviceSize uniformBufferSize = sizeof(glm::vec4);

            vk::BufferCreateInfo createInfo;
            createInfo.setSize(uniformBufferSize)
                .setUsage(vk::BufferUsageFlagBits::eUniformBuffer)
                .setSharingMode(vk::SharingMode::eExclusive);
            uniformBuffer = vulkanRHI->CreateBuffer(createInfo, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, uniformBufferMemory);

            mappedUniformBuffer = vulkanRHI->MapMemory(uniformBufferMemory, 0, uniformBufferSize);

            const glm::vec4 uniformData{color, 0.0f};
            memcpy(mappedUniformBuffer, &uniformData, sizeof(uniformData));
        }

        mainCameraPass->PreparePassData(uniformBuffer);
    }
    
    void RenderSystem::Quit()
    {
        vulkanRHI->WaitIdle();

        vulkanRHI->UnmapMemory(uniformBufferMemory);
        vulkanRHI->DestroyBuffer(uniformBuffer);
        vulkanRHI->FreeMemory(uniformBufferMemory);

        renderResource->Quit();
        mainCameraPass->Quit();
        vulkanRHI->Quit();
    }

    void RenderSystem::Render()
    {
        vulkanRHI->PrepareBeforeRender();

        UpdateUniformBuffer();

        mainCameraPass->Draw();

        vulkanRHI->SubmitRendering();
    }

    void RenderSystem::UpdateUniformBuffer()
    {
        const float time = static_cast<float>(glfwGetTime());
        const float phase = 2.0f * glm::pi<float>() / 3.0f;

        color.r = 0.5f + 0.5f * std::sin(time);
        color.g = 0.5f + 0.5f * std::sin(time + phase);
        color.b = 0.5f + 0.5f * std::sin(time + 2.0f * phase);

        const glm::vec4 uniformData{color, 0.0f};
        memcpy(mappedUniformBuffer, &uniformData, sizeof(uniformData));
    }
}