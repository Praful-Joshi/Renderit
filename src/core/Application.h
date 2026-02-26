#pragma once

#include "Window.h"
#include <memory>

namespace Core {

class Application {
public:
    Application();
    virtual ~Application() = default;

    // Non-copyable, non-movable — there is exactly one Application
    Application(const Application&)            = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&)                 = delete;
    Application& operator=(Application&&)      = delete;

    void run();

protected:
    // Override these in subclasses to add behaviour without touching the loop
    virtual void onInit()   {}
    virtual void onUpdate(float deltaTime) {}
    virtual void onRender() {}
    virtual void onShutdown() {}

    // Input — we'll expand this into its own InputManager later
    bool isKeyPressed(int glfwKey) const;

    std::unique_ptr<Window> m_window;

private:
    void mainLoop();
    float calcDeltaTime();

    float m_lastFrameTime = 0.0f;
};

} // namespace Core