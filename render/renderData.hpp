#pragma once

#define GLM_ENABLE_EXPERIMENTAL 

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

namespace XrayEngine
{
    enum RenderDomain : uint8_t
    {
        eRenderDomainUndefined, 
		eRenderDomainForwardLighting,
		eRenderDomainDeferredLighting,
		eRenderDomainTransparency,
		eRenderDomainPostProcess,
		eRenderDomainCount,
    };

    struct BoundingBox
    {
        glm::vec3 min{0.0f};
        glm::vec3 max{0.0f};
        bool valid{false};
    };

    struct MeshVertex
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec4 tangent;
        glm::vec2 texCoord;

        static std::vector<vk::VertexInputAttributeDescription> GetAttributeDescriptions()
        {
            std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;
            attributeDescriptions.resize(4);

            attributeDescriptions[0].setBinding(0)
                .setLocation(0)
                .setFormat(vk::Format::eR32G32B32Sfloat)
                .setOffset(0);
            attributeDescriptions[1].setBinding(0)
                .setLocation(1)
                .setFormat(vk::Format::eR32G32B32Sfloat)
                .setOffset(sizeof(glm::vec3));
            attributeDescriptions[2].setBinding(0)
                .setLocation(2)
                .setFormat(vk::Format::eR32G32B32A32Sfloat)
                .setOffset(offsetof(MeshVertex, tangent));
            attributeDescriptions[3].setBinding(0)
                .setLocation(3)
                .setFormat(vk::Format::eR32G32Sfloat)
                .setOffset(offsetof(MeshVertex, texCoord));

            return attributeDescriptions;
        }

        static std::vector<vk::VertexInputBindingDescription> GetBindingDescriptions()
        {
            std::vector<vk::VertexInputBindingDescription> bindingDescriptions;
            bindingDescriptions.resize(1);

            bindingDescriptions[0].setBinding(0)
                .setInputRate(vk::VertexInputRate::eVertex)
                .setStride(sizeof(MeshVertex));

            return bindingDescriptions;
        }

        bool operator==(const MeshVertex& other) const
        {
            return position == other.position
                && normal == other.normal
                && tangent == other.tangent
                && texCoord == other.texCoord;
        }
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

    enum class MaterialAlphaMode : uint32_t
    {
        eOpaque,
        eMask,
        eBlend,
    };

    struct TextureInfoData
    {
        int textureIndex{-1};
        uint32_t texCoord{0};
    };

    template <typename T>
    struct Parameter
    {
        std::string name;
        T value;
    };

    struct MaterialData
    {
        using Scalar = float;
        using Vector = glm::vec4;

        std::string name;

        RenderDomain domain;

        float alphaCutoff{0.5f};
        MaterialAlphaMode alphaMode{MaterialAlphaMode::eOpaque};
        uint32_t doubleSided{0};

        std::vector<Parameter<Scalar>> scalars;
        std::vector<Parameter<Vector>> vectors;
        std::vector<Parameter<TextureInfoData>> textures;

        void setScalar(const std::string &key, Scalar value) { setParameter(scalars, key, value); }
        void setVector(const std::string &key, const Vector &value) { setParameter(vectors, key, value); }
        void setTexture(const std::string &key, const TextureInfoData &value) { setParameter(textures, key, value); }

        bool hasScalar(const std::string &key) const { return findParameter(scalars, key) != nullptr; }
        bool hasVector(const std::string &key) const { return findParameter(vectors, key) != nullptr; }
        bool hasTexture(const std::string &key) const { return findParameter(textures, key) != nullptr; }

        Scalar scalar(const std::string &key, Scalar fallback = Scalar{0.0f}) const
        {
            const Scalar *value = findParameter(scalars, key);
            return value != nullptr ? *value : fallback;
        }

        Vector vector(const std::string &key, const Vector &fallback = Vector{0.0f}) const
        {
            const Vector *value = findParameter(vectors, key);
            return value != nullptr ? *value : fallback;
        }

        TextureInfoData texture(const std::string &key) const
        {
            const TextureInfoData *value = findParameter(textures, key);
            return value != nullptr ? *value : TextureInfoData{};
        }

    private:
        template <typename T>
        static void setParameter(std::vector<Parameter<T>> &parameters, const std::string &key, const T &value)
        {
            for (Parameter<T> &parameter : parameters)
            {
                if (parameter.name == key)
                {
                    parameter.value = value;
                    return;
                }
            }
            parameters.push_back(Parameter<T>{key, value});
        }

        template <typename T>
        static const T *findParameter(const std::vector<Parameter<T>> &parameters, const std::string &key)
        {
            for (const Parameter<T> &parameter : parameters)
            {
                if (parameter.name == key)
                {
                    return &parameter.value;
                }
            }
            return nullptr;
        }
    };

    struct RenderEntity
    {
        glm::mat4 modelMatrix{1.0f};

        int meshIndex{-1};
        int materialIndex{-1};

        BoundingBox boundingBox;

        std::vector<uint32_t> children;
    };
}
