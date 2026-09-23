#pragma once

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

namespace XrayEngine
{
    struct MeshVertex
    {
        glm::vec2 position;
        glm::vec2 texCoord;

        static std::vector<vk::VertexInputAttributeDescription> GetAttributeDescriptions()
        {
            std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;
            attributeDescriptions.resize(2);

            attributeDescriptions[0].setBinding(0)
                .setLocation(0)
                .setFormat(vk::Format::eR32G32Sfloat)
                .setOffset(0);
            attributeDescriptions[1].setBinding(0)
                .setLocation(1)
                .setFormat(vk::Format::eR32G32Sfloat)
                .setOffset(sizeof(glm::vec2));

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
    };
}