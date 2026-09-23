#include <GLFW/glfw3.h>

#include "render/renderSystem.hpp"

using namespace XrayEngine;

struct WindowCreateInfo
{
    int width{800};
    int height{800};
    const char *title{"XrayEngine"};
    bool isFullScreen{false};
};

void InitializeWindow(GLFWwindow *&window, const WindowCreateInfo& createInfo);

int main()
{
    WindowCreateInfo createInfo;

    GLFWwindow* window;
    InitializeWindow(window, createInfo);

    RenderSystemInitInfo renderSystemInitInfo;
    renderSystemInitInfo.window = window;
    renderSystemInitInfo.windowWidth = (float)createInfo.width;
    renderSystemInitInfo.windowHeight = (float)createInfo.height;
    std::shared_ptr<RenderSystem> renderSystem = std::make_shared<RenderSystem>();
    renderSystem->Initialize(renderSystemInitInfo);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        renderSystem->Render();
    }

    renderSystem->Quit();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

void InitializeWindow(GLFWwindow*& window, const WindowCreateInfo& createInfo)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window = glfwCreateWindow(createInfo.width, createInfo.height, createInfo.title, nullptr, nullptr);

    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return;
    }
}
