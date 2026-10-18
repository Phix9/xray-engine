#pragma once

#include <unordered_map>
#include <vulkan/vulkan.hpp>
#include <stb_image.h>
#include <tiny_obj_loader.h>
#include <glm/glm.hpp>

#include "render/interface/vulkanRHI.hpp"
#include "render/mesh.hpp"
#include "render/material.hpp"
#include "render/renderScene.hpp"
#include "render/renderCamera.hpp"

namespace XrayEngine
{
    struct RenderResourceInitInfo
    {
        std::shared_ptr<VulkanRHI> vulkanRHI;
    };

    struct MeshData
    {
        std::vector<MeshVertex> vertices;
        std::vector<uint32_t> indices;
    };

    struct TextureData
    {
        uint32_t width{0};
        uint32_t height{0};
        void* pixels{nullptr};
        vk::Format format{vk::Format::eUndefined};
    };

    struct RenderMaterialData
    {
        TextureData baseColor;
        TextureData metallic;
        TextureData roughness;
        TextureData normal;
        TextureData occlusion;
    };
    
    struct Mesh
    {
        uint32_t vertexCount{0};
        uint32_t indexCount{0};

        vk::Buffer vertexBuffer;
        vk::DeviceMemory vertexBufferMemory;

        vk::Buffer indexBuffer;
        vk::DeviceMemory indexBufferMemory;
    };

    struct Texture
    {
        vk::Image image;
        vk::ImageView imageView;
        vk::DeviceMemory memory;
    };

    struct RenderMaterial
    {
        Texture baseColor;
        Texture metallic;
        Texture roughness;
        Texture normal;
        Texture occlusion;
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

    class RenderResource
    {
    public:
        std::shared_ptr<VulkanRHI> vulkanRHI;

        std::vector<Mesh> meshes;
        std::vector<Texture> textures;

        Mesh mesh;
        RenderMaterial material;
        Storage storage;

        MeshPerFrameStorageBufferObject meshPerFrameStorageBufferObject;

        void Initialize(RenderResourceInitInfo initInfo);
        void Quit();
        MeshData LoadMesh(std::string filePath);
        TextureData LoadTexture(std::string filePath, bool isSRGB);
        RenderMaterialData LoadMaterial(Material material);
        void CreateMeshes(const std::vector<MeshVertex> &meshData);
        void CreateIndices(const std::vector<uint32_t> &indices);
        void CreateMaterial(const RenderMaterialData &materialData);
        void CreateTextures(const TextureData &textureData, Texture &targetTexture);
        void CreateStorageBuffer();
        void ResetRingBufferOffset();
        void UpdatePerFrameBuffer(std::shared_ptr<RenderScene> renderScene, std::shared_ptr<RenderCamera> camera);
        void TransitionImageLayout(vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
    };
}