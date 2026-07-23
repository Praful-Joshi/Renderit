#pragma once

#include "Window.h"
#include <memory>

namespace Core
{

/**
 * @class Application
 * @brief Base class representing the main application lifecycle.
 *
 * @details
 * Responsible for:
 * - Creating and owning the Window
 * - Managing the main render loop
 * - Providing overridable hooks for initialization, update, render, and shutdown
 *
 * Lifecycle:
 *   constructor → run()
 *       → onInit()
 *       → mainLoop()
 *           → onUpdate()
 *           → onRender()
 *       → onShutdown()
 *
 * This class is non-copyable and non-movable.
 * There must exist exactly one Application instance.
 */
class Application
{
  public:
    /**
     * @brief Constructs the Application and creates the main window.
     *
     * Initializes GLFW, GLAD, and creates the OpenGL context
     * through the Window abstraction.
     *
     * @throws std::runtime_error if window or context creation fails.
     */
    Application();

    virtual ~Application() = default;

    Application(const Application&)            = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&)                 = delete;
    Application& operator=(Application&&)      = delete;

    /**
     * @brief Starts the application lifecycle.
     *
     * Calls:
     *  - onInit()
     *  - main loop
     *  - onShutdown()
     */
    void run();

  protected:
    /**
     * @brief Called once before entering the main loop.
     */
    virtual void onInit()
    {
    }

    /**
     * @brief Called once per frame before rendering.
     *
     * @param deltaTime Time elapsed since previous frame in seconds.
     */
    virtual void onUpdate(float deltaTime)
    {
    }

    /**
     * @brief Called once per frame to perform rendering.
     *
     * OpenGL state should already be valid.
     */
    virtual void onRender()
    {
    }

    /**
     * @brief Called once after exiting the main loop.
     */
    virtual void onShutdown()
    {
    }

    /**
     * @brief Returns whether a key is currently pressed.
     *
     * @param glfwKey GLFW key enum (e.g., GLFW_KEY_ESCAPE)
     * @return True if key is pressed.
     */
    bool isKeyPressed(int glfwKey) const;

    /// Owning pointer to the main window.
    std::unique_ptr<Window> m_window;

  private:
    /// Executes the main render loop.
    void mainLoop();

    /**
     * @brief Calculates frame delta time.
     *
     * @return Time in seconds since last frame.
     */
    float calcDeltaTime();

    float m_lastFrameTime = 0.0f;
};

} // namespace Core