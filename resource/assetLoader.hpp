#pragma once

#include <tiny_gltf.h>
#include <glm/gtc/type_ptr.hpp>

#include "render/renderData.hpp"

namespace XrayEngine
{
    struct GltfPrimitive
    {
        int meshDataIndex{-1};
        int material{-1};
        BoundingBox boundingBox{};
    };

    struct GltfMesh
    {
        std::string name;
        std::vector<GltfPrimitive> primitives;
        BoundingBox boundingBox{};
    };

    struct GltfTexture
    {
        int textureDataIndex{-1};
    };

    struct GltfMaterial
    {
        std::string name;
        glm::vec4 baseColorFactor{1.0f, 1.0f, 1.0f, 1.0f};
        int baseColorTexture{-1};
        float metallicFactor{1.0f};
        float roughnessFactor{1.0f};
        int metallicRoughnessTexture{-1};

        int normalTexture{-1};
        float normalScale{1.0f};

        int occlusionTexture{-1};
        float occlusionStrength{1.0f};

        int emissiveTexture{-1};
        glm::vec3 emissiveFactor{0.0f};
    };

    struct GltfNode
    {
        std::string name;
        int index;
        std::vector<int> children;
        int mesh{-1};
        glm::mat4 matrix{1.0f};
    };

    struct GltfScene
    {
        int node{-1};
    };

    struct GltfModel
    {
        GltfScene scene;
        std::vector<GltfNode> nodes;
        std::vector<GltfMesh> meshes;
        std::vector<GltfTexture> textures;
        std::vector<GltfMaterial> materials;
    };

    class AssetLoader
    {
    public:
        static void LoadModel(std::string filePath, std::vector<uint32_t> &renderObjects, std::vector<RenderEntity> &renderEntities, std::vector<MeshData> &meshDatas, std::vector<MaterialData> &materialDatas, std::vector<TextureData> &textureDatas);

    private:
        static GltfModel LoadGltfModel(std::string filePath, std::vector<MaterialData> &materialDatas, std::vector<MeshData> &meshDatas, std::vector<TextureData> &textureDatas);
        static GltfTexture LoadGltfTexture(const tinygltf::Model &gltfModel, uint32_t index, bool isSRGB, std::vector<TextureData> &textureDatas);
        static GltfMesh LoadGltfMesh(const tinygltf::Model &gltfModel, uint32_t index, std::vector<MeshData> &meshDatas);
        static GltfNode LoadGltfNode(const tinygltf::Model &gltfModel, uint32_t index);
        static GltfMaterial LoadGltfMaterial(const tinygltf::Model &gltfModel, uint32_t index, const std::vector<GltfTexture> &textures, std::vector<MaterialData> &materialDatas);
        static int ProcessGltfModel(const GltfModel &model, int nodeIndex, const glm::mat4 &parentWorld, std::vector<RenderEntity> &renderEntities);
    };
}