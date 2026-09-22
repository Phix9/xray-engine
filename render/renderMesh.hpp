#pragma once

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

namespace XrayEngine
{
    struct MeshVertex
    {
        glm::vec2 position;

        static vk::VertexInputAttributeDescription GetAttributeDescription()
        {
            vk::VertexInputAttributeDescription attributeDescription;

            attributeDescription.setBinding(0)
                .setLocation(0)
                .setFormat(vk::Format::eR32G32Sfloat)
                .setOffset(0);

            return attributeDescription;
        }

        static vk::VertexInputBindingDescription GetBindingDescription()
        {
            vk::VertexInputBindingDescription bindingDescription;

            bindingDescription.setBinding(0)
                .setInputRate(vk::VertexInputRate::eVertex)
                .setStride(sizeof(MeshVertex));

            return bindingDescription;
        }
    };
}