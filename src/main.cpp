#include "core/Application.h"
#include <GLFW/glfw3.h>
#include <iostream>

// Concrete application — overrides the virtual hooks
class MyApp : public Core::Application {
protected:
    void onInit() override {
        std::cout << "[App] Initialized\n";
    }

    void onUpdate(float deltaTime) override {
        // Close on Escape
        if (isKeyPressed(GLFW_KEY_ESCAPE))
            glfwSetWindowShouldClose(m_window->getNativeWindow(), true);

        // Show FPS in window title
        m_frameCount++;
        m_timeAccum += deltaTime;
        if (m_timeAccum >= 1.0f) {
            float fps = static_cast<float>(m_frameCount) / m_timeAccum;
            m_window->setTitle("OpenGL Renderer | FPS: " + std::to_string(static_cast<int>(fps)));
            m_frameCount = 0;
            m_timeAccum  = 0.0f;
        }
    }

    void onRender() override {
        // Nothing to render yet — next step
    }

    void onShutdown() override {
        std::cout << "[App] Shutdown\n";
    }

private:
    int   m_frameCount = 0;
    float m_timeAccum  = 0.0f;
};

int main() {
    try {
        MyApp app;
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "[Fatal] " << e.what() << "\n";
        return -1;
    }
    return 0;
}