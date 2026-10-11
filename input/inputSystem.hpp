#pragma once

#include <glfw/glfw3.h>
#include <glm/glm.hpp>

namespace XrayEngine
{
    struct InputSystemInitInfo
    {
        GLFWwindow *window;
    };

    struct InputData
    {
        glm::vec3 moveDirection{0.0f, 0.0f, 0.0f};
        glm::vec2 mouseMovementDelta{0.0f, 0.0f};
    };

    class InputSystem
    {
    public:
        void Initialize(InputSystemInitInfo initInfo);
        void ProcessInput();
        void ResetInputData();
        InputData GetInputData();

    private:
        GLFWwindow *window{nullptr};
        InputData inputData;

        bool isFirstMouseMovement{true};
        glm::vec2 lastMousePosition{0.0f};

        static void MouseMovementCallback(GLFWwindow *window, double xposIn, double yposIn); 
    };
}