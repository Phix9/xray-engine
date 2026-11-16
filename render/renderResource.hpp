#pragma once

#include <unordered_map>
#include <vulkan/vulkan.hpp>
#include <tiny_obj_loader.h>
#include <glm/glm.hpp>

#include "resource/assetLoader.hpp"
#include "render/interface/vulkanRHI.hpp"
#include "render/renderData.hpp"
#include "render/renderScene.hpp"
#include "render/renderCamera.hpp"

namespace XrayEngine
{
    struct RenderResourceInitInfo
    {
        std::shared_ptr<VulkanRHI> vulkanRHI;
    };
    
    struct RHIMesh
    {
        uint32_t vertexCount{0};
        uint32_t indexCount{0};

        vk::Buffer vertexBuffer;
        vk::DeviceMemory vertexBufferMemory;

        vk::Buffer indexBuffer;
        vk::DeviceMemory indexBufferMemory;
    };

    struct RHITexture
    {
        vk::Image image;
        vk::ImageView imageView;
        vk::DeviceMemory memory;
    };

    struct RHIMaterial 
    {   
        std::vector<int> textureReferences;
        
        vk::Buffer materialUniformBuffer;
        vk::DeviceMemory materialUniformBufferMemory;

        vk::DescriptorSetLayout descriptorSetLayout;
        vk::DescriptorSet descriptorSet;
    };

    struct Storage
    {
        uint32_t minUniformBufferOffsetAlignment;
        uint32_t minStorageBufferOffsetAlignment;
        uint32_t maxStorageBufferRange;

        std::vector<uint32_t> ringBufferBegins;
        std::vector<uint32_t> ringBufferEnds;
        std::vector<uint32_t> ringBufferSizes;

        vk::Buffer ringBuffer;
        vk::DeviceMemory ringBufferMemory;

        void* mappedMemory;
    };

    struct DirectionalLightObject
    {
        glm::vec3 direcion;
        float direction_padding;
        glm::vec3 color;
        float color_padding;
    };

    struct PointLightObject
    {
        glm::vec3 position;
        float radius;
        glm::vec3 intensity;
        float intensity_padding;
    };

    struct MeshPerFrameStorageBufferObject
    {
        glm::mat4 projectViewMatrix;
        glm::mat4 inverseProjectViewMatrix;
        glm::vec3 cameraPosition;
        float cameraPosition_padding{0.0f};
        glm::vec3 ambientLight;
        float ambientLight_padding{0.0f};
        uint32_t pointLightCount;
        uint32_t pointLightCount_padding1;
        uint32_t pointLightCount_padding2;
        uint32_t pointLightCount_padding3;
        PointLightObject pointLights[16];
        DirectionalLightObject directionalLight;
    };

    struct MeshPerMaterialUniformBufferObject
    {
        std::vector<float> uniforms;
    };

    class RenderResource
    {
    public:
        std::shared_ptr<VulkanRHI> vulkanRHI;

        std::vector<RHITexture> texturePool;

        std::vector<RHIMesh> meshes;
        std::vector<RHIMaterial> materials;

        std::vector<uint32_t> renderObjects;
        std::vector<RenderEntity> renderEntities;

        std::vector<MeshData> meshDatas;
        std::vector<TextureData> textureDatas;
        std::vector<MaterialData> materialDatas;

        vk::Sampler sampler;

        Storage storage;

        MeshPerFrameStorageBufferObject meshPerFrameStorageBufferObject;

        void Initialize(RenderResourceInitInfo initInfo);
        void Quit();
        void CreateModel(std::string filePath);
        void CreateStorageBuffer();
        void ResetRingBufferOffset();
        void UpdatePerFramebuffer(std::shared_ptr<RenderScene> renderScene, std::shared_ptr<RenderCamera> camera);
        void TransitionImageLayout(vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);

    private:
        RHIMesh CreateMesh(const MeshData &meshData);
        RHITexture CreateTexture(const TextureData &textureData);
        RHIMaterial CreateMaterial(const MaterialData &materialData);
    };
}