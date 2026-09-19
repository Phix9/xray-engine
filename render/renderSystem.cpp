#include "render/renderSystem.hpp"

#include <cmath>
#include <cstring>
#include <glm/gtc/constants.hpp>

namespace XrayEngine
{
    const std::array<MeshVertex, 3> meshData{
        glm::vec2{-0.5, 0.5},
        glm::vec2{0.5, 0.5},
        glm::vec2{0.0, -0.5},
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

        MainCameraPassInitInfo mainCameraPassInitInfo;
        mainCameraPassInitInfo.vulkanRHI = vulkanRHI;
        mainCameraPass = std::make_shared<MainCameraPass>();
        mainCameraPass->Initialize(&mainCameraPassInitInfo);

        // initialize vertex buffer
        {
            vk::BufferCreateInfo createInfo;
            createInfo.setSize(sizeof(meshData))
                .setUsage(vk::BufferUsageFlagBits::eTransferSrc)
                .setSharingMode(vk::SharingMode::eExclusive);
            hostVertexBuffer = vulkanRHI->CreateBuffer(createInfo, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, hostVertexBufferMemory);

            createInfo.setSize(sizeof(meshData))
                .setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer)
                .setSharingMode(vk::SharingMode::eExclusive);
            deviceVertexBuffer = vulkanRHI->CreateBuffer(createInfo, vk::MemoryPropertyFlagBits::eDeviceLocal, deviceVertexBufferMemory);

            void *ptr = vulkanRHI->MapMemory(hostVertexBufferMemory, 0, sizeof(meshData));
            memcpy(ptr, meshData.data(), sizeof(meshData));
            vulkanRHI->UnmapMemory(hostVertexBufferMemory);
            
            vk::CommandBuffer commandBuffer = vulkanRHI->BeginOneTimeCommandBuffer();
            
            vk::BufferCopy region;
            region.setSrcOffset(0)
            .setDstOffset(0)
            .setSize(sizeof(meshData));
            
            vulkanRHI->CommandCopyBuffer(commandBuffer, hostVertexBuffer, deviceVertexBuffer, region);
            
            vulkanRHI->EndOneTimeCommandBuffer(commandBuffer);
            
            vulkanRHI->DestroyBuffer(hostVertexBuffer);
            vulkanRHI->FreeMemory(hostVertexBufferMemory);
        }
        
        // initialize uniform buffer
        {
            vk::BufferCreateInfo createInfo;
            createInfo.setSize(sizeof(glm::vec4))
            .setUsage(vk::BufferUsageFlagBits::eUniformBuffer)
            .setSharingMode(vk::SharingMode::eExclusive);
            uniformBuffer = vulkanRHI->CreateBuffer(createInfo, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, uniformBufferMemory);

            mappedUniformBuffer = vulkanRHI->MapMemory(uniformBufferMemory, 0, sizeof(glm::vec4));

            const glm::vec4 uniformData{color, 0.0f};
            memcpy(mappedUniformBuffer, &uniformData, sizeof(uniformData));
        }

        mainCameraPass->PreparePassData(deviceVertexBuffer, uniformBuffer);
    }
    
    void RenderSystem::Quit()
    {
        vulkanRHI->WaitIdle();

        vulkanRHI->DestroyBuffer(deviceVertexBuffer);
        vulkanRHI->FreeMemory(deviceVertexBufferMemory);

        vulkanRHI->UnmapMemory(uniformBufferMemory);
        vulkanRHI->DestroyBuffer(uniformBuffer);
        vulkanRHI->FreeMemory(uniformBufferMemory);

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