#include "core/Application.h"
#include "renderer/Shader.h"
#include "renderer/Buffer.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <memory>

class CubeApp : public Core::Application {
protected:

    void onInit() override {
        glEnable(GL_DEPTH_TEST);

        m_shader = std::make_unique<Renderer::Shader>(
            "../assets/shaders/basic.vert",
            "../assets/shaders/basic.frag"
        );

        // ── Vertex Data ───────────────────────────────────────────────────────
        //
        // 24 vertices: 6 faces × 4 corners each.
        // Even though a cube has only 8 unique positions, each face needs its
        // own 4 vertices because they carry different colors. A vertex shared
        // between two faces can only have one color — so we duplicate positions
        // and give each copy the color of its face.
        //
        // This is the exact same reason OBJ files duplicate vertices —
        // positions may be shared but UVs and normals differ per face.
        //
        // Layout per vertex: [ x, y, z, r, g, b ]  (stride = 24 bytes)
        //
        // Face colors:
        //   Front  = red      Back   = cyan
        //   Top    = green    Bottom = magenta
        //   Right  = blue     Left   = yellow

        std::vector<float> vertices = {

            // ── FRONT FACE (z = +0.5) — red ──────────────────────────────────
            //      position               color (r,  g,  b)
            -0.5f,  0.5f,  0.5f,    1.0f, 0.2f, 0.2f,  // 0: top-left
             0.5f,  0.5f,  0.5f,    1.0f, 0.2f, 0.2f,  // 1: top-right
             0.5f, -0.5f,  0.5f,    1.0f, 0.2f, 0.2f,  // 2: bottom-right
            -0.5f, -0.5f,  0.5f,    1.0f, 0.2f, 0.2f,  // 3: bottom-left

            // ── BACK FACE (z = -0.5) — cyan ───────────────────────────────────
             0.5f,  0.5f, -0.5f,    0.2f, 1.0f, 1.0f,  // 4: top-left  (from back)
            -0.5f,  0.5f, -0.5f,    0.2f, 1.0f, 1.0f,  // 5: top-right (from back)
            -0.5f, -0.5f, -0.5f,    0.2f, 1.0f, 1.0f,  // 6: bottom-right
             0.5f, -0.5f, -0.5f,    0.2f, 1.0f, 1.0f,  // 7: bottom-left

            // ── TOP FACE (y = +0.5) — green ───────────────────────────────────
            -0.5f,  0.5f, -0.5f,    0.2f, 1.0f, 0.2f,  // 8:  top-left  (from top)
             0.5f,  0.5f, -0.5f,    0.2f, 1.0f, 0.2f,  // 9:  top-right
             0.5f,  0.5f,  0.5f,    0.2f, 1.0f, 0.2f,  // 10: bottom-right
            -0.5f,  0.5f,  0.5f,    0.2f, 1.0f, 0.2f,  // 11: bottom-left

            // ── BOTTOM FACE (y = -0.5) — magenta ──────────────────────────────
            -0.5f, -0.5f,  0.5f,    1.0f, 0.2f, 1.0f,  // 12: top-left  (from bottom)
             0.5f, -0.5f,  0.5f,    1.0f, 0.2f, 1.0f,  // 13: top-right
             0.5f, -0.5f, -0.5f,    1.0f, 0.2f, 1.0f,  // 14: bottom-right
            -0.5f, -0.5f, -0.5f,    1.0f, 0.2f, 1.0f,  // 15: bottom-left

            // ── RIGHT FACE (x = +0.5) — blue ──────────────────────────────────
             0.5f,  0.5f,  0.5f,    0.2f, 0.2f, 1.0f,  // 16: top-left  (from right)
             0.5f,  0.5f, -0.5f,    0.2f, 0.2f, 1.0f,  // 17: top-right
             0.5f, -0.5f, -0.5f,    0.2f, 0.2f, 1.0f,  // 18: bottom-right
             0.5f, -0.5f,  0.5f,    0.2f, 0.2f, 1.0f,  // 19: bottom-left

            // ── LEFT FACE (x = -0.5) — yellow ─────────────────────────────────
            -0.5f,  0.5f, -0.5f,    1.0f, 1.0f, 0.2f,  // 20: top-left  (from left)
            -0.5f,  0.5f,  0.5f,    1.0f, 1.0f, 0.2f,  // 21: top-right
            -0.5f, -0.5f,  0.5f,    1.0f, 1.0f, 0.2f,  // 22: bottom-right
            -0.5f, -0.5f, -0.5f,    1.0f, 1.0f, 0.2f,  // 23: bottom-left
        };

        // ── Index Data ────────────────────────────────────────────────────────
        //
        // 36 indices: 6 faces × 2 triangles × 3 vertices
        //
        // Each face is a quad (4 vertices). We split it into 2 triangles.
        // The pattern for each face (local indices 0-3):
        //
        //   0 ──── 1
        //   │  \   │
        //   │   \  │
        //   3 ──── 2
        //
        //   Triangle 1: 0, 1, 2
        //   Triangle 2: 0, 2, 3
        //
        // Each face's 4 vertices start at: face_index * 4

        std::vector<uint32_t> indices;
        for (uint32_t face = 0; face < 6; ++face) {
            uint32_t base = face * 4;
            indices.push_back(base + 0);
            indices.push_back(base + 1);
            indices.push_back(base + 2);
            indices.push_back(base + 0);
            indices.push_back(base + 2);
            indices.push_back(base + 3);
        }

        // ── Upload to GPU ─────────────────────────────────────────────────────
        std::vector<Renderer::VertexAttribute> attributes = {
            { 0, 3,  0 },   // a_position: 3 floats at byte offset 0
            { 1, 3, 12 },   // a_color:    3 floats at byte offset 12
        };
        GLsizei stride = 6 * sizeof(float);

        m_buffer = std::make_unique<Renderer::Buffer>();
        m_buffer->uploadVertices(vertices, attributes, stride);
        m_buffer->uploadIndices(indices);

        // ── Transforms ────────────────────────────────────────────────────────
        m_view = glm::lookAt(
            glm::vec3(1.5f, 1.5f, 3.0f),  // slightly up and to the right
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );

        float aspect = static_cast<float>(m_window->getWidth()) /
                       static_cast<float>(m_window->getHeight());
        m_projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    }

    void onUpdate(float deltaTime) override {
        if (isKeyPressed(GLFW_KEY_ESCAPE))
            glfwSetWindowShouldClose(m_window->getNativeWindow(), true);

        // Rotate around a diagonal axis so all 6 faces become visible
        m_rotation += 30.0f * deltaTime;
        m_model = glm::rotate(
            glm::mat4(1.0f),
            glm::radians(m_rotation),
            glm::vec3(1.0f, 1.0f, 0.0f)
        );
    }

    void onRender() override {
        m_shader->bind();
        m_shader->setMat4("u_model",      m_model);
        m_shader->setMat4("u_view",       m_view);
        m_shader->setMat4("u_projection", m_projection);

        m_buffer->bind();

        // glDrawElements uses the index buffer — fetches vertices by index
        // instead of sequentially. The last arg is a byte offset into the EBO.
        glDrawElements(GL_TRIANGLES, m_buffer->indexCount(), GL_UNSIGNED_INT, 0);

        m_buffer->unbind();
        m_shader->unbind();
    }

    void onShutdown() override {
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
        CubeApp app;
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "[Fatal] " << e.what() << "\n";
        return -1;
    }
    return 0;
}