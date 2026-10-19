#pragma once

#include <memory>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "input/inputSystem.hpp"

namespace XrayEngine
{
    static constexpr float PI = 3.14159265358979323846264338327950288f;

    struct RenderCameraInitInfo
    {
        std::shared_ptr<InputSystem> inputSystem;
    };

    class RenderCamera
    {
    public:
        static const glm::vec3 X, Y, Z;

        void Initialize(RenderCameraInitInfo initInfo);
        void Update();

        glm::vec3 GetPosition();
        glm::mat4 GetViewMatrix();
        glm::mat4 GetProjectMatrix();

    private:
        std::shared_ptr<InputSystem> inputSystem;

        glm::vec3 position{0.0f, 0.5f, 0.0f};
        glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
        glm::vec3 upAxis{Z};

        float zNear{0.01f};
        float zFar{1000.0f};

        float fov{90.0f};
        float aspect{1.0f};

        float moveSpeed{0.001f};
        float rotateSpeed{0.0025f};

        float yaw{PI};
        float pitch{0.0f};

        void Move();
        void Rotate();
    };

    inline const glm::vec3 RenderCamera::X = {1.0f, 0.0f, 0.0f};
    inline const glm::vec3 RenderCamera::Y = {0.0f, 1.0f, 0.0f};
    inline const glm::vec3 RenderCamera::Z = {0.0f, 0.0f, 1.0f};
}