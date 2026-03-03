#include "core/Application.h"
#include "renderer/Shader.h"
#include "renderer/Model.h"
#include "scene/AssimpLoader.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <memory>

class CottageApp : public Core::Application {
protected:

    void onInit() override {
        glEnable(GL_DEPTH_TEST);

        m_shader = std::make_unique<Renderer::Shader>(
            "../assets/shaders/basic.vert",
            "../assets/shaders/basic.frag"
        );

        // Load model via Assimp — works with OBJ, FBX, GLTF, and 40+ other formats.
        // The MTL file is parsed automatically, textures loaded relative to model path.
        m_model = Scene::AssimpLoader::load("../models/cottage/cottage_obj.obj");

        m_model->setScale(0.1f);        // Camera
        m_view = glm::lookAt(
            glm::vec3(5.0f, 3.0f, 3.0f),
            glm::vec3(0.0f, 1.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );

        float aspect = static_cast<float>(m_window->getWidth()) /
                       static_cast<float>(m_window->getHeight());
        m_projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    }

    void onUpdate(float deltaTime) override {
        if (isKeyPressed(GLFW_KEY_ESCAPE))
            glfwSetWindowShouldClose(m_window->getNativeWindow(), true);

        m_rotation += 20.0f * deltaTime;
        m_model->setRotation(m_rotation, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    void onRender() override {
        m_shader->bind();
        m_shader->setMat4("u_view",       m_view);
        m_shader->setMat4("u_projection", m_projection);

        // Model::draw() sets u_model and draws all meshes.
        // Each mesh binds its own material (textures + uniforms) internally.
        m_model->draw(*m_shader);

        m_shader->unbind();
    }

    void onShutdown() override {
        std::cout << "[App] Shutdown\n";
    }

private:
    std::unique_ptr<Renderer::Shader> m_shader;
    std::unique_ptr<Renderer::Model>  m_model;

    glm::mat4 m_view       = glm::mat4(1.0f);
    glm::mat4 m_projection = glm::mat4(1.0f);
    float     m_rotation   = 0.0f;
};

int main() {
    try {
        CottageApp app;
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "[Fatal] " << e.what() << "\n";
        return -1;
    }
    return 0;
}