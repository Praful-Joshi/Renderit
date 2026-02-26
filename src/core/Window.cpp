#include "Window.h"
#include <iostream>
#include <stdexcept>

namespace Core {

Window::Window(const WindowConfig& config) : m_config(config) {
    glfwSetErrorCallback(errorCallback);

    if (!glfwInit())
        throw std::runtime_error("Failed to initialize GLFW");

    // Request OpenGL 3.3 Core Profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Required on macOS

    m_glWindow = glfwCreateWindow(
        m_config.width,
        m_config.height,
        m_config.title.c_str(),
        nullptr, nullptr
    );

    if (!m_glWindow) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(m_glWindow);

    // Store a pointer to this Window object inside GLFW so static callbacks
    // can reach back into our instance
    glfwSetWindowUserPointer(m_glWindow, this);
    glfwSetFramebufferSizeCallback(m_glWindow, framebufferSizeCallback);

    if (m_config.vsync)
        glfwSwapInterval(1); // lock to monitor refresh rate
    else
        glfwSwapInterval(0);

    // Initialize GLAD after context is current
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        throw std::runtime_error("Failed to initialize GLAD");

    // Print context info
    std::cout << "[Window] OpenGL " << glGetString(GL_VERSION) << "\n";
    std::cout << "[Window] GPU: "   << glGetString(GL_RENDERER) << "\n";
}

Window::~Window() {
    if (m_glWindow)
        glfwDestroyWindow(m_glWindow);
    glfwTerminate();
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(m_glWindow);
}

void Window::swapBuffers() const {
    glfwSwapBuffers(m_glWindow);
}

void Window::pollEvents() const {
    glfwPollEvents();
}

void Window::setTitle(const std::string& title) {
    m_config.title = title;
    glfwSetWindowTitle(m_glWindow, title.c_str());
}

float Window::getAspectRatio() const {
    return static_cast<float>(m_config.width) / static_cast<float>(m_config.height);
}

// Static callback — GLFW calls this when the window is resized
void Window::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    // Retrieve our Window instance from the GLFW user pointer
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    self->m_config.width  = width;
    self->m_config.height = height;
    glViewport(0, 0, width, height);
}

void Window::errorCallback(int error, const char* description) {
    std::cerr << "[GLFW Error " << error << "] " << description << "\n";
}

} // namespace Core