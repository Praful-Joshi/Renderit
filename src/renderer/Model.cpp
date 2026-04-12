#include "Model.h"

namespace Renderer {

Model::Model(std::vector<std::unique_ptr<Mesh>> meshes)
    : m_meshes(std::move(meshes))
{
    rebuildModelMatrix();
}

void Model::draw(const Shader& shader) const {
    shader.setMat4("u_model", m_modelMatrix);
    for (const auto& mesh : m_meshes)
        mesh->draw(shader);
}

void Model::setPosition(const glm::vec3& pos) {
    m_position = pos;
    rebuildModelMatrix();
}

void Model::setRotation(float angleDegrees, const glm::vec3& axis) {
    m_angleDeg     = angleDegrees;
    m_rotationAxis = axis;
    rebuildModelMatrix();
}

void Model::setScale(const glm::vec3& scale) {
    m_scale = scale;
    rebuildModelMatrix();
}

void Model::setScale(float uniformScale) {
    m_scale = glm::vec3(uniformScale);
    rebuildModelMatrix();
}

void Model::setBaseRotation(float angleDegrees, const glm::vec3& axis) {
    m_baseRotation = glm::rotate(glm::mat4(1.0f), glm::radians(angleDegrees), axis);
    rebuildModelMatrix();
}

void Model::rebuildModelMatrix() {
    // Order: scale → baseRotation → spin → translate
    // baseRotation fixes the model's rest orientation (applied first, in model space).
    // The per-frame spin is layered on top in world space.
    glm::mat4 m = glm::mat4(1.0f);
    m = glm::translate(m, m_position);
    if (m_angleDeg != 0.0f)
        m = glm::rotate(m, glm::radians(m_angleDeg), m_rotationAxis);
    m = m * m_baseRotation;   // base orientation fix applied after spin
    m = glm::scale(m, m_scale);
    m_modelMatrix = m;
}

} // namespace Renderer