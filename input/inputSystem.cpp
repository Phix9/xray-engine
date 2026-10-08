#include "input/inputSystem.hpp"

namespace XrayEngine
{
    void InputSystem::Initialize(InputSystemInitInfo initInfo)
    {
        window = initInfo.window;

        glfwSetWindowUserPointer(window, this);
        glfwSetCursorPosCallback(window, MouseMovementCallback);
    }

    void InputSystem::ProcessInput()
    {
        inputData.mouseMovementDelta = glm::vec2(0.0f);

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, true);
        }

        glm::vec3 moveDirection{0.0f, 0.0f, 0.0f};

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        {
           moveDirection += glm::vec3(1.0f, 0.0f, 0.0f);
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        {
           moveDirection += glm::vec3(-1.0f, 0.0f, 0.0f);
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        {
           moveDirection += glm::vec3(0.0f, -1.0f, 0.0f);
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        {
           moveDirection += glm::vec3(0.0f, 1.0f, 0.0f);
        }
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        {
            moveDirection += glm::vec3(0.0f, 0.0f, 1.0f);
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        {
            moveDirection += glm::vec3(0.0f, 0.0f, -1.0f);
        }

        if(glm::dot(moveDirection, moveDirection) > 0.0f)
        {
            inputData.moveDirection = glm::normalize(moveDirection);
        }
        else
        {
            inputData.moveDirection = glm::vec3{0.0f};
        }
    }

    void InputSystem::MouseMovementCallback(GLFWwindow *window, double xposIn, double yposIn)
    {
        InputSystem *inputSystem = static_cast<InputSystem*>(glfwGetWindowUserPointer(window));
        if (inputSystem == nullptr)
        {
            return;
        }

        glm::vec2 mousePosition{(float)xposIn, (float)yposIn};

        if (inputSystem->isFirstMouseMovement)
        {
            inputSystem->isFirstMouseMovement = false;
            inputSystem->lastMousePosition = mousePosition;
            return;
        }

        glm::vec2 delta = mousePosition - inputSystem->lastMousePosition;
        inputSystem->lastMousePosition = mousePosition;

        inputSystem->inputData.mouseMovementDelta = delta;
    }
}