#pragma once

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <string>

namespace Core
{

/**
 * @struct WindowConfig
 * @brief Configuration parameters used to create a Window.
 *
 * @details
 * This struct defines the initial state of the GLFW window
 * and OpenGL context settings such as resolution, title,
 * and vertical synchronization.
 */
struct WindowConfig
{
    int         width  = 800;      ///< Initial window width in pixels.
    int         height = 600;      ///< Initial window height in pixels.
    std::string title  = "OpenGL"; ///< Window title displayed in title bar.
    bool        vsync  = true;     ///< Enables vertical synchronization if true.
};

/**
 * @class Window
 * @brief RAII wrapper around a GLFW window and OpenGL context.
 *
 * @details
 * Responsibilities:
 *  - Initialize GLFW
 *  - Create an OpenGL 3.3 Core profile context
 *  - Load OpenGL functions via GLAD
 *  - Manage swap buffers and event polling
 *  - Handle framebuffer resize events
 *
 * Ownership:
 *  - Owns a GLFWwindow*
 *  - Non-copyable (unique window owner)
 *  - Movable
 *
 * Lifetime:
 *  - GLFW is initialized in constructor
 *  - GLFW is terminated in destructor
 *
 * @throws std::runtime_error if GLFW or GLAD initialization fails.
 */
class Window
{
  public:
    /**
     * @brief Creates a window and initializes the OpenGL context.
     *
     * @param config Window configuration parameters.
     *
     * @throws std::runtime_error if:
     *  - GLFW fails to initialize
     *  - Window creation fails
     *  - GLAD fails to load OpenGL functions
     */
    explicit Window(const WindowConfig& config);

    /**
     * @brief Destroys the window and terminates GLFW.
     */
    ~Window();

    Window(const Window&)            = delete;
    Window& operator=(const Window&) = delete;

    Window(Window&&)            = default;
    Window& operator=(Window&&) = default;

    /**
     * @brief Checks whether the window should close.
     *
     * @return True if a close event has been triggered.
     */
    bool shouldClose() const;

    /**
     * @brief Swaps the front and back buffers.
     */
    void swapBuffers() const;

    /**
     * @brief Polls and processes pending window events.
     */
    void pollEvents() const;

    /**
     * @brief Sets a new window title.
     *
     * @param title New window title.
     */
    void setTitle(const std::string& title);

    /**
     * @return Current window width in pixels.
     */
    int getWidth() const
    {
        return m_config.width;
    }

    /**
     * @return Current window height in pixels.
     */
    int getHeight() const
    {
        return m_config.height;
    }

    /**
     * @brief Computes current aspect ratio.
     *
     * @return width / height.
     */
    float getAspectRatio() const;

    /**
     * @brief Returns underlying GLFW window pointer.
     *
     * @warning Exposes raw pointer — use carefully.
     */
    GLFWwindow* getNativeWindow() const
    {
        return m_glWindow;
    }

  private:
    /**
     * @brief GLFW framebuffer resize callback.
     *
     * Updates internal width/height and adjusts OpenGL viewport.
     */
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);

    /**
     * @brief GLFW error callback.
     */
    static void errorCallback(int error, const char* description);

    GLFWwindow*  m_glWindow = nullptr; ///< Owned GLFW window handle.
    WindowConfig m_config;             ///< Current window configuration.
};

} // namespace Core