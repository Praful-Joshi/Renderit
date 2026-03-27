#include "core/Application.h"
#include "renderer/Shader.h"
#include "renderer/Model.h"
#include "renderer/Buffer.h"
#include "scene/AssimpLoader.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <memory>
#include <vector>

class CottageApp : public Core::Application {
protected:

    void onInit() override {
        glEnable(GL_DEPTH_TEST);

        // ── Shaders ───────────────────────────────────────────────────────────
        m_shader = std::make_unique<Renderer::Shader>(
            "../assets/shaders/basic.vert",
            "../assets/shaders/pbr.frag"
        );
        m_unlitShader = std::make_unique<Renderer::Shader>(
            "../assets/shaders/unlit.vert",
            "../assets/shaders/unlit.frag"
        );

        // ── Load model ────────────────────────────────────────────────────────
        m_model = Scene::AssimpLoader::load("../models/flesh/flesh_blob.fbx");

        // ── Light cube geometry ───────────────────────────────────────────────
        // A simple unit cube — 8 unique positions, 36 indices.
        // We only need positions for the unlit shader (no normals/UVs needed).
        std::vector<float> cubeVerts = {
            -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f, 0.5f,-0.5f, -0.5f, 0.5f,-0.5f,
            -0.5f,-0.5f, 0.5f,  0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
        };
        std::vector<uint32_t> cubeIdx = {
            0,1,2, 0,2,3,   // back
            4,5,6, 4,6,7,   // front
            0,4,7, 0,7,3,   // left
            1,5,6, 1,6,2,   // right
            3,2,6, 3,6,7,   // top
            0,1,5, 0,5,4,   // bottom
        };

        // Upload cube with only position attribute (location 0)
        std::vector<Renderer::VertexAttribute> cubeAttribs = {{ 0, 3, 0 }};
        m_lightCubeBuffer = std::make_unique<Renderer::Buffer>();
        m_lightCubeBuffer->uploadVertices(cubeVerts, cubeAttribs, 3 * sizeof(float));
        m_lightCubeBuffer->uploadIndices(cubeIdx);

        // Model setup
        m_model->setScale(2.5f);
        m_model->setRotation(90.0, glm::vec3(.0, 1., .0));

        // ── Camera ────────────────────────────────────────────────────────────
        m_cameraPos = glm::vec3(0.0f, 5.0f, 8.0f);
        m_view = glm::lookAt(
            m_cameraPos,
            glm::vec3(0.0f, 2.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );

        float aspect = static_cast<float>(m_window->getWidth()) /
                       static_cast<float>(m_window->getHeight());
        m_projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    }

    void onUpdate(float deltaTime) override {
        if (isKeyPressed(GLFW_KEY_ESCAPE))
            glfwSetWindowShouldClose(m_window->getNativeWindow(), true);

        // Rotate the cottage slowly
        m_rotation += 20.0f * deltaTime;
        m_model->setRotation(m_rotation, glm::vec3(0.0f, 1.0f, 0.0f));

        // Orbit the light around the scene so you can see the lighting change
        // as it moves — makes diffuse and specular effects very obvious.
        m_lightAngle += 60.0f * deltaTime;  // 60 degrees per second
        float rad = glm::radians(m_lightAngle);
        m_lightPos = glm::vec3(
            std::cos(rad) * m_lightOrbitRadius,
            m_lightHeight,
            std::sin(rad) * m_lightOrbitRadius
        );
    }

    void onRender() override {

        // ── Cottage — Blinn-Phong lit ─────────────────────────────────────────
        m_shader->bind();

        // Camera + projection
        m_shader->setMat4("u_view",       m_view);
        m_shader->setMat4("u_projection", m_projection);
        m_shader->setVec3("u_cameraPos",  m_cameraPos);

        // Point light uniforms
        m_shader->setVec3 ("u_light.position",  m_lightPos);
        m_shader->setVec3 ("u_light.color",     m_lightColor);
        m_shader->setFloat("u_light.constant",  1.0f);
        m_shader->setFloat("u_light.linear",    0.09f);
        m_shader->setFloat("u_light.quadratic", 0.032f);

        // Draw model — sets u_model internally per mesh
        m_model->draw(*m_shader);

        m_shader->unbind();

        // ── Light visualizer cube — unlit ─────────────────────────────────────
        // A small white/yellow cube floating at the light's world position.
        // Rendered without lighting so it always appears bright regardless
        // of the scene lighting — it IS the light source visually.
        m_unlitShader->bind();
        m_unlitShader->setMat4("u_view",       m_view);
        m_unlitShader->setMat4("u_projection", m_projection);
        m_unlitShader->setVec3("u_color",      m_lightColor);

        // Scale down to a small cube and position at the light location
        glm::mat4 lightModel = glm::translate(glm::mat4(1.0f), m_lightPos);
        lightModel = glm::scale(lightModel, glm::vec3(0.2f));
        m_unlitShader->setMat4("u_model", lightModel);

        m_lightCubeBuffer->bind();
        glDrawElements(GL_TRIANGLES, m_lightCubeBuffer->indexCount(),
                       GL_UNSIGNED_INT, 0);
        m_lightCubeBuffer->unbind();

        m_unlitShader->unbind();
    }

    void onShutdown() override {
        std::cout << "[App] Shutdown\n";
    }

private:
    // Shaders
    std::unique_ptr<Renderer::Shader> m_shader;
    std::unique_ptr<Renderer::Shader> m_unlitShader;

    // Scene
    std::unique_ptr<Renderer::Model>  m_model;
    std::unique_ptr<Renderer::Buffer> m_lightCubeBuffer;

    // Camera
    glm::vec3 m_cameraPos  = glm::vec3(0.0f, 3.0f, 8.0f);
    glm::mat4 m_view       = glm::mat4(1.0f);
    glm::mat4 m_projection = glm::mat4(1.0f);

    // Light — orbits around the scene so lighting effects are clearly visible
    glm::vec3 m_lightPos         = glm::vec3(3.0f, 3.0f, 3.0f);
    glm::vec3 m_lightColor       = glm::vec3(1.0f, 0.95f, 0.8f); // warm white
    float     m_lightAngle       = 0.0f;
    float     m_lightOrbitRadius = 4.0f;
    float     m_lightHeight      = 3.5f;

    float m_rotation = 0.0f;
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