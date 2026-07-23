#include "Application.h"
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <iostream>

namespace Core
{
Application::Application()
{
    WindowConfig config;
    config.width  = 800;
    config.height = 600;
    config.title  = "OpenGL Renderer";
    config.vsync  = true;

    m_window = std::make_unique<Window>(config);
}

void Application::run()
{
    onInit();
    mainLoop();
    onShutdown();
}

void Application::mainLoop()
{
    while (!m_window->shouldClose())
    {
        float deltaTime = calcDeltaTime();

        m_window->pollEvents();
        onUpdate(deltaTime);

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        onRender();

        m_window->swapBuffers();
    }
}

float Application::calcDeltaTime()
{
    float currentTime = static_cast<float>(glfwGetTime());
    float delta       = currentTime - m_lastFrameTime;
    m_lastFrameTime   = currentTime;
    return delta;
}

bool Application::isKeyPressed(int glfwKey) const
{
    return glfwGetKey(m_window->getNativeWindow(), glfwKey) == GLFW_PRESS;
}

} // namespace Core