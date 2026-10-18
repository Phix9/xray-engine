#pragma once

#define GLM_ENABLE_EXPERIMENTAL 
#include <functional>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

namespace XrayEngine
{
    struct MeshVertex
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec3 tangent;
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
                .setFormat(vk::Format::eR32G32B32Sfloat)
                .setOffset(sizeof(glm::vec3) * 2);
            attributeDescriptions[3].setBinding(0)
                .setLocation(3)
                .setFormat(vk::Format::eR32G32Sfloat)
                .setOffset(sizeof(glm::vec3) * 3);

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
}

namespace std
{
    template<>
    struct hash<XrayEngine::MeshVertex>
    {
        size_t operator()(const XrayEngine::MeshVertex& vertex) const noexcept
        {
            const size_t positionHash = hash<glm::vec3>{}(vertex.position);
            const size_t normalHash = hash<glm::vec3>{}(vertex.normal);
            const size_t tangentHash = hash<glm::vec3>{}(vertex.tangent);
            const size_t texCoordHash = hash<glm::vec2>{}(vertex.texCoord);

            size_t seed = positionHash;
            seed ^= normalHash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= tangentHash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= texCoordHash + 0x9e3779b9 + (seed << 6) + (seed >> 2);

            return seed;
        }
    };
}