#include "core/Application.h"
#include "renderer/Shader.h"
#include "renderer/Model.h"
#include "renderer/Buffer.h"
#include "renderer/ShadowMap.h"
#include "scene/AssimpLoader.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
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
            "../assets/shaders/basic.frag"
        );
        m_unlitShader = std::make_unique<Renderer::Shader>(
            "../assets/shaders/unlit.vert",
            "../assets/shaders/unlit.frag"
        );
        m_shadowShader = std::make_unique<Renderer::Shader>(
            "../assets/shaders/shadow.vert",
            "../assets/shaders/shadow.frag"
        );

        // ── Shadow map ────────────────────────────────────────────────────────
        m_shadowMap = std::make_unique<Renderer::ShadowMap>();

        // ── Load model ────────────────────────────────────────────────────────
        m_model = Scene::AssimpLoader::load("../models/cottage/cottage_obj.obj");

        m_model->setScale(0.1f);
        // m_model->setBaseRotation(-90.0f, glm::vec3(1.0f, 0.0f, 0.0f)); 

        // ── Light cube geometry ───────────────────────────────────────────────
        std::vector<float> cubeVerts = {
            -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f, 0.5f,-0.5f, -0.5f, 0.5f,-0.5f,
            -0.5f,-0.5f, 0.5f,  0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
        };
        std::vector<uint32_t> cubeIdx = {
            0,1,2, 0,2,3,
            4,5,6, 4,6,7,
            0,4,7, 0,7,3,
            1,5,6, 1,6,2,
            3,2,6, 3,6,7,
            0,1,5, 0,5,4,
        };
        std::vector<Renderer::VertexAttribute> cubeAttribs = {{ 0, 3, 0 }};
        m_lightCubeBuffer = std::make_unique<Renderer::Buffer>();
        m_lightCubeBuffer->uploadVertices(cubeVerts, cubeAttribs, 3 * sizeof(float));
        m_lightCubeBuffer->uploadIndices(cubeIdx);

        // ── Camera ────────────────────────────────────────────────────────────
        m_cameraPos = glm::vec3(0.0f, 3.0f, 8.0f);
        m_view = glm::lookAt(
            m_cameraPos,
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

        // Orbit light
        m_lightAngle += 60.0f * deltaTime;
        float rad = glm::radians(m_lightAngle);
        m_lightPos = glm::vec3(
            std::cos(rad) * m_lightOrbitRadius,
            m_lightHeight,
            std::sin(rad) * m_lightOrbitRadius
        );

        // ── Light space matrix ────────────────────────────────────────────────
        // We treat the point light as a spot aimed at the scene origin.
        // Near/far planes: 1.0–25.0 covers our scene with good depth precision.
        glm::mat4 lightProj = glm::perspective(
            glm::radians(90.0f),  // wide FOV so the whole scene fits in shadow map
            1.0f,                 // square shadow map → aspect 1:1
            1.0f, 25.0f           // near / far
        );
        glm::mat4 lightView = glm::lookAt(
            m_lightPos,
            glm::vec3(0.0f, 0.0f, 0.0f),  // always look at scene centre
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
        m_lightSpaceMatrix = lightProj * lightView;
    }

    void onRender() override {

        // ════════════════════════════════════════════════════════════════
        // Pass 1 — Shadow pass: render scene depth from light's viewpoint
        // ════════════════════════════════════════════════════════════════
        m_shadowMap->bindForWriting();
        glViewport(0, 0, Renderer::ShadowMap::WIDTH, Renderer::ShadowMap::HEIGHT);

        // Cull front faces during shadow pass — reduces "peter panning"
        // (shadow detaching from caster) and self-shadowing artefacts.
        glCullFace(GL_FRONT);

        m_shadowShader->bind();
        m_shadowShader->setMat4("u_lightSpaceMatrix", m_lightSpaceMatrix);
        m_model->draw(*m_shadowShader);  // only u_model + u_lightSpaceMatrix needed
        m_shadowShader->unbind();

        glCullFace(GL_BACK);  // restore for main pass
        m_shadowMap->unbindWriting();

        // ════════════════════════════════════════════════════════════════
        // Pass 2 — Main pass: render scene with shadow map
        // ════════════════════════════════════════════════════════════════
        int w = m_window->getWidth();
        int h = m_window->getHeight();
        glViewport(0, 0, w, h);

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Bind shadow map depth texture to unit 5
        // (units 0-4 are reserved for material maps)
        m_shadowMap->bindTextureToUnit(5);

        m_shader->bind();
        m_shader->setMat4("u_view",             m_view);
        m_shader->setMat4("u_projection",       m_projection);
        m_shader->setVec3("u_cameraPos",        m_cameraPos);
        m_shader->setMat4("u_lightSpaceMatrix", m_lightSpaceMatrix);
        m_shader->setInt ("u_shadowMap",        5);

        m_shader->setVec3 ("u_light.position",  m_lightPos);
        m_shader->setVec3 ("u_light.color",     m_lightColor);
        m_shader->setFloat("u_light.constant",  1.0f);
        m_shader->setFloat("u_light.linear",    0.09f);
        m_shader->setFloat("u_light.quadratic", 0.032f);

        m_model->draw(*m_shader);
        m_shader->unbind();

        // ── Light visualizer cube ─────────────────────────────────────────────
        m_unlitShader->bind();
        m_unlitShader->setMat4("u_view",       m_view);
        m_unlitShader->setMat4("u_projection", m_projection);
        m_unlitShader->setVec3("u_color",      m_lightColor);

        glm::mat4 lightModel = glm::translate(glm::mat4(1.0f), m_lightPos);
        lightModel = glm::scale(lightModel, glm::vec3(0.2f));
        m_unlitShader->setMat4("u_model", lightModel);

        m_lightCubeBuffer->bind();
        glDrawElements(GL_TRIANGLES, m_lightCubeBuffer->indexCount(), GL_UNSIGNED_INT, 0);
        m_lightCubeBuffer->unbind();
        m_unlitShader->unbind();
    }

    void onShutdown() override {
        std::cout << "[App] Shutdown\n";
    }

private:
    // Shaders
    std::unique_ptr<Renderer::Shader>    m_shader;
    std::unique_ptr<Renderer::Shader>    m_unlitShader;
    std::unique_ptr<Renderer::Shader>    m_shadowShader;

    // Scene
    std::unique_ptr<Renderer::Model>     m_model;
    std::unique_ptr<Renderer::Buffer>    m_lightCubeBuffer;
    std::unique_ptr<Renderer::ShadowMap> m_shadowMap;

    // Camera
    glm::vec3 m_cameraPos  = glm::vec3(0.0f, 3.0f, 8.0f);
    glm::mat4 m_view       = glm::mat4(1.0f);
    glm::mat4 m_projection = glm::mat4(1.0f);

    // Light
    glm::vec3 m_lightPos         = glm::vec3(3.0f, 2.5f, 2.0f);
    glm::vec3 m_lightColor       = glm::vec3(1.0f, 0.95f, 0.8f);
    float     m_lightAngle       = 0.0f;
    float     m_lightOrbitRadius = 3.0f;
    float     m_lightHeight      = 2.5f;
    glm::mat4 m_lightSpaceMatrix = glm::mat4(1.0f);

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
