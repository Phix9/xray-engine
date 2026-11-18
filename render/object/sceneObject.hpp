#pragma once

#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace XrayEngine
{
    struct Transform
    {
        glm::vec3 position{0.0f};
        glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
        glm::vec3 scale{1.0f};

        glm::mat4 GetModelMatrix() const
        {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
            model *= glm::mat4_cast(rotation);
            model = glm::scale(model, scale);
            return model;
        }

        glm::mat3 GetNormalMatrix() const
        {
            return glm::transpose(glm::inverse(glm::mat3(GetModelMatrix())));
        }
    };

    struct SceneObject
    {
        std::string name;
        Transform transform;

        uint32_t renderObject;
    };
}