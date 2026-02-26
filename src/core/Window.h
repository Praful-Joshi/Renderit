#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>

namespace Core {

struct WindowConfig {
    int width        = 800;
    int height       = 600;
    std::string title = "OpenGL";
    bool vsync       = true;
};

class Window {
public:
    explicit Window(const WindowConfig& config);
    ~Window();

    // Non-copyable — there should only ever be one owner of a GLFWwindow
    Window(const Window&)            = delete;
    Window& operator=(const Window&) = delete;

    // Movable
    Window(Window&&)            = default;
    Window& operator=(Window&&) = default;

    bool        shouldClose() const;
    void        swapBuffers() const;
    void        pollEvents() const;
    void        setTitle(const std::string& title);

    int         getWidth()  const { return m_config.width; }
    int         getHeight() const { return m_config.height; }
    float       getAspectRatio() const;
    GLFWwindow* getNativeWindow() const { return m_glWindow; }

private:
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void errorCallback(int error, const char* description);

    GLFWwindow*  m_glWindow = nullptr;
    WindowConfig m_config;
};

} // namespace Core