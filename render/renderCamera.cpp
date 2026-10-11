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
        const glm::vec3 inputDirection = inputSystem->GetInputData().moveDirection;

        const glm::vec3 forward = rotation * RenderCamera::Y;
        const glm::vec3 right = rotation * RenderCamera::X;

        const glm::vec3 worldMoveDirection = forward * inputDirection.x +
                                             right * inputDirection.y +
                                             upAxis * inputDirection.z;

        position += worldMoveDirection * moveSpeed;
    }

    void RenderCamera::Rotate()
    {
        const glm::vec2 mouseMovementDelta = inputSystem->GetInputData().mouseMovementDelta;

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

        glm::mat4 view{1.0f};
        view[0][0] = s.x;
        view[1][0] = s.y;
        view[2][0] = s.z;
        view[3][0] = glm::dot(-s, position);
        view[0][1] = u.x;
        view[1][1] = u.y;
        view[2][1] = u.z;
        view[3][1] = glm::dot(-u, position);
        view[0][2] = -f.x;
        view[1][2] = -f.y;
        view[2][2] = -f.z;
        view[3][2] = glm::dot(f, position);

        return view;
    }
    
    glm::mat4 RenderCamera::GetProjectMatrix()
    {
        float t = std::tan(glm::radians(fov) / 2.0f);

        glm::mat4 project{0.0f};
        project[0][0] = 1.0f / (aspect * t);
        project[1][1] = -1.0f / t;
        project[2][2] = zFar / (zNear - zFar);
        project[3][2] = -(zFar * zNear) / (zFar - zNear);
        project[2][3] = -1.0f;

        return project;
    }
}