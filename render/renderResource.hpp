#pragma once

#include <vulkan/vulkan.hpp>
#include <stb_image.h>
#include <glm/glm.hpp>

#include "render/interface/vulkanRHI.hpp"
#include "render/renderMesh.hpp"
#include "render/renderCamera.hpp"

namespace XrayEngine
{
    struct RenderResourceInitInfo
    {
        std::shared_ptr<VulkanRHI> vulkanRHI;
    };

    struct TextureData
    {
        uint32_t width;
        uint32_t height;
        void* pixels;
    };

    struct Mesh
    {
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

    struct MeshPerFrameStorageBufferObject
    {
        glm::mat4 projectViewMatrix;
        glm::vec3 cameraPosition;
        float cameraPositionPadding{0.0f};
    };

    class RenderResource
    {
    public:
        std::shared_ptr<VulkanRHI> vulkanRHI;

        std::vector<Mesh> meshes;
        std::vector<Texture> textures;

        Mesh mesh;
        Texture texture;

        MeshPerFrameStorageBufferObject meshPerFrameStorageBufferObject;

        void Initialize(RenderResourceInitInfo initInfo);
        void Quit();
        TextureData LoadTexture(std::string filePath);
        void CreateMeshes(const std::vector<MeshVertex> &meshData);
        void CreateIndices(const std::vector<uint16_t> &indices);
        void CreateTextures(const TextureData &textureData);
        void UpdatePerFrameBuffer(std::shared_ptr<RenderCamera> camera);
        void TransitionImageLayout(vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
    };
}