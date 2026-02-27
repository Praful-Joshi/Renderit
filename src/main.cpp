#include "core/Application.h"
#include "renderer/Shader.h"
#include "renderer/Buffer.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <memory>

class TriangleApp : public Core::Application {
protected:

    void onInit() override {
        glEnable(GL_DEPTH_TEST);

        // ── Shader ────────────────────────────────────────────────────────────
        // Paths are relative to where you run the binary (the build/ directory)
        // so we go up one level to reach assets/
        m_shader = std::make_unique<Renderer::Shader>(
            "../assets/shaders/basic.vert",
            "../assets/shaders/basic.frag"
        );

        // ── Geometry ──────────────────────────────────────────────────────────
        // One vertex = position (xyz) + color (rgb)
        // Laid out flat: [ x, y, z, r, g, b,   x, y, z, r, g, b, ... ]
        //
        // These are in model space — a triangle centered around the origin,
        // sitting on the XZ plane. Y is up.
        std::vector<float> vertices = {
        //   position              color
        //   x      y      z      r     g     b
             0.0f,  0.5f,  0.0f,  1.0f, 0.3f, 0.3f,  // top    — red
            -0.5f, -0.5f,  0.0f,  0.3f, 1.0f, 0.3f,  // left   — green
             0.5f, -0.5f,  0.0f,  0.3f, 0.3f, 1.0f,  // right  — blue
        };

        // Describe the layout of one vertex to the GPU:
        //   attribute 0: position, 3 floats, starts at byte 0
        //   attribute 1: color,    3 floats, starts at byte 12 (3 floats × 4 bytes)
        std::vector<Renderer::VertexAttribute> attributes = {
            { 0, 3,  0 },   // a_position
            { 1, 3, 12 },   // a_color
        };

        GLsizei stride = 6 * sizeof(float); // 6 floats per vertex = 24 bytes

        m_buffer = std::make_unique<Renderer::Buffer>();
        m_buffer->uploadVertices(vertices, attributes, stride);

        // ── Transforms ────────────────────────────────────────────────────────
        // Model: identity — triangle sits at world origin, original scale
        m_model = glm::mat4(1.0f);

        // View: camera at (0,0,3) looking at origin
        // We use Z=3 (not Z=5) since the triangle is flat on screen — no depth
        m_view = glm::lookAt(
            glm::vec3(0.0f, 0.0f, 3.0f),  // eye
            glm::vec3(0.0f, 0.0f, 0.0f),  // center
            glm::vec3(0.0f, 1.0f, 0.0f)   // up
        );

        // Projection: 45° fov, current aspect ratio, near/far planes
        float aspect = static_cast<float>(m_window->getWidth()) /
                       static_cast<float>(m_window->getHeight());
        m_projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    }

    void onUpdate(float deltaTime) override {
        if (isKeyPressed(GLFW_KEY_ESCAPE))
            glfwSetWindowShouldClose(m_window->getNativeWindow(), true);

        // Rotate the model around Y axis over time — demonstrates the model matrix
        // changing each frame while view and projection stay fixed
        m_rotation += 45.0f * deltaTime; // 45 degrees per second
        m_model = glm::rotate(
            glm::mat4(1.0f),
            glm::radians(m_rotation),
            glm::vec3(0.0f, 1.0f, 0.0f)  // Y axis
        );
    }

    void onRender() override {
        m_shader->bind();

        // Upload the three matrices as uniforms — these travel to the vertex
        // shader and are used in: gl_Position = projection * view * model * pos
        m_shader->setMat4("u_model",      m_model);
        m_shader->setMat4("u_view",       m_view);
        m_shader->setMat4("u_projection", m_projection);

        m_buffer->bind();

        // Draw 3 vertices as one triangle
        glDrawArrays(GL_TRIANGLES, 0, 3);

        m_buffer->unbind();
        m_shader->unbind();
    }

    void onShutdown() override {
        // unique_ptr destructors handle GPU cleanup via Buffer/Shader destructors
        std::cout << "[App] Clean shutdown\n";
    }

private:
    std::unique_ptr<Renderer::Shader> m_shader;
    std::unique_ptr<Renderer::Buffer> m_buffer;

    glm::mat4 m_model      = glm::mat4(1.0f);
    glm::mat4 m_view       = glm::mat4(1.0f);
    glm::mat4 m_projection = glm::mat4(1.0f);

    float m_rotation = 0.0f;
};

int main() {
    try {
        TriangleApp app;
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "[Fatal] " << e.what() << "\n";
        return -1;
    }
    return 0;
}