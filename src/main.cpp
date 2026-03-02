#include "core/Application.h"
#include "renderer/Mesh.h"
#include "renderer/Shader.h"
#include "scene/ObjLoader.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <memory>
#include <vector>

class ObjApp : public Core::Application
{
  protected:
    void onInit() override
    {
        glEnable(GL_DEPTH_TEST);

        // ── Load shader ───────────────────────────────────────────────────────
        m_shader = std::make_unique<Renderer::Shader>("../assets/shaders/basic.vert",
                                                      "../assets/shaders/basic.frag");

        // ── Load OBJ ──────────────────────────────────────────────────────────
        // Path is relative to the build/ directory where the binary runs.
        // Your cottage obj folder should be at the project root.
        m_meshes = Scene::ObjLoader::load("../85-cottage_obj/cottage_obj.obj");

        if (m_meshes.empty())
        {
            throw std::runtime_error("Failed to load cottage OBJ. Check the path.");
        }

        // ── Camera and projection ─────────────────────────────────────────────
        // The cottage may be large — we position the camera further back.
        // We'll adjust once we see the model.
        m_view = glm::lookAt(glm::vec3(0.0f, 3.0f, 100.0f), // camera position
                             glm::vec3(0.0f, 1.0f, 0.0f),   // look at slightly above origin
                             glm::vec3(0.0f, 1.0f, 0.0f)    // up
        );

        float aspect =
            static_cast<float>(m_window->getWidth()) / static_cast<float>(m_window->getHeight());
        m_projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    }

    void onUpdate(float deltaTime) override
    {
        if (isKeyPressed(GLFW_KEY_ESCAPE))
            glfwSetWindowShouldClose(m_window->getNativeWindow(), true);

        // Slow rotation so we can inspect all sides
        m_rotation += 20.0f * deltaTime;
        m_model = glm::rotate(glm::mat4(1.0f), glm::radians(m_rotation),
                              glm::vec3(0.0f, 1.0f, 0.0f) // rotate around Y (vertical axis)
        );
    }

    void onRender() override
    {
        m_shader->bind();
        m_shader->setMat4("u_model", m_model);
        m_shader->setMat4("u_view", m_view);
        m_shader->setMat4("u_projection", m_projection);

        // Draw all meshes in the model.
        // Each mesh came from a different 'usemtl' section in the OBJ.
        for (const auto& mesh : m_meshes)
        {
            mesh->draw(*m_shader);
        }

        m_shader->unbind();
    }

    void onShutdown() override
    {
        std::cout << "[App] Shutdown\n";
    }

  private:
    std::unique_ptr<Renderer::Shader>            m_shader;
    std::vector<std::unique_ptr<Renderer::Mesh>> m_meshes;

    glm::mat4 m_model      = glm::mat4(1.0f);
    glm::mat4 m_view       = glm::mat4(1.0f);
    glm::mat4 m_projection = glm::mat4(1.0f);

    float m_rotation = 0.0f;
};

int main()
{
    try
    {
        ObjApp app;
        app.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "[Fatal] " << e.what() << "\n";
        return -1;
    }
    return 0;
}