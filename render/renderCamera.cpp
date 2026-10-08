#include <render/renderCamera.hpp>

namespace XrayEngine
{
    void RenderCamera::Initialize(RenderCameraInitInfo initInfo)
    {
        inputSystem = initInfo.inputSystem;
    }

    void RenderCamera::Update()
    {
        Move();
        Rotate();
    }

    void RenderCamera::Move()
    {
        position += (inputSystem->GetInputData().moveDirection * moveSpeed);
    }

    void RenderCamera::Rotate()
    {
        const glm::vec2 mouseMovementDelta = inputSystem->GetInputData().mouseMovementDelta;
        if (mouseMovementDelta == glm::vec2{0.0f})
        {
            return;
        }

        yaw -= mouseMovementDelta.x * rotateSpeed;
        pitch -= mouseMovementDelta.y * rotateSpeed;

        pitch = glm::clamp(pitch, -glm::radians(89.0f), glm::radians(89.0f));

        rotation = glm::normalize(glm::angleAxis(yaw, upAxis) * glm::angleAxis(pitch, RenderCamera::X));
    }

    glm::vec3 RenderCamera::GetPosition()
    {
        return position;
    }

    glm::mat4 RenderCamera::GetViewMatrix()
    {
        const glm::vec3 u = rotation * RenderCamera::Z;
        const glm::vec3 f = rotation * RenderCamera::Y;
        const glm::vec3 s = glm::cross(f, u);

        glm::mat4 view{0.0f};
        view[0][0] = s.x;
        view[0][1] = s.y;
        view[0][2] = s.z;
        view[0][3] = glm::dot(-s, position);
        view[1][0] = u.x;
        view[1][1] = u.y;
        view[1][2] = u.z;
        view[1][3] = glm::dot(-u, position);
        view[2][0] = -f.x;
        view[2][1] = -f.y;
        view[2][2] = -f.z;
        view[2][3] = glm::dot(f, position);

        return view;
    }
    
    glm::mat4 RenderCamera::GetProjectMatrix()
    {
        float t = std::tan(fov / 2.0f);

        glm::mat4 project{0.0f};
        project[0][0] = 1.0f / (aspect * t);
        project[1][1] = 1 / t;
        project[2][2] = -zFar / (zNear - zFar);
        project[2][3] = -(zFar * zNear) / (zFar - zNear);
        project[3][2] = -1.0f;

        return project;
    }
}