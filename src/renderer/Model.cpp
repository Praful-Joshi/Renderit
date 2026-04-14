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
    // Correct transform order (applied right-to-left by GPU):
    //   1. baseRotation — fixes model's rest orientation in model space (innermost)
    //   2. spin         — rotates around world Y axis
    //   3. translate    — moves to world position
    //   4. scale        — uniform scale
    //
    // Key insight: baseRotation must be the INNERMOST operation so it always
    // corrects the model's local axes regardless of the current spin angle.
    // If it were applied after the spin, its axis would rotate with the model
    // and produce wrong normals (black patches) at certain orientations.
    glm::mat4 m = glm::mat4(1.0f);
    m = glm::translate(m, m_position);
    m = glm::rotate(m, glm::radians(m_angleDeg), m_rotationAxis);
    m = glm::scale(m, m_scale);
    m_modelMatrix = m * m_baseRotation;  // base fix is innermost — model space
}

} // namespace Renderer