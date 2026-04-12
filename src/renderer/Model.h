#pragma once

#include "Mesh.h"
#include "Shader.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <memory>
#include <string>

namespace Renderer {

// A Model is a collection of Meshes loaded from a single model file.
// It holds the model-space transform and delegates draw calls to its meshes.
//
// Assimp may split one model file into many meshes (one per material).
// Model reunites them under one object you can place in a scene.
class Model {
public:
    explicit Model(std::vector<std::unique_ptr<Mesh>> meshes);

    // Draw all meshes with the given shader.
    // Sets u_model uniform before drawing.
    void draw(const Shader& shader) const;

    // ── Transform API ─────────────────────────────────────────────────────────
    void setPosition(const glm::vec3& pos);
    void setRotation(float angleDegrees, const glm::vec3& axis);
    void setScale   (const glm::vec3& scale);
    void setScale   (float uniformScale);

    // Fix-up rotation applied before the spin — use this to correct a model's
    // rest orientation (e.g. an FBX exported on its side from Blender).
    // setRotation() / the per-frame spin are applied on top of this.
    void setBaseRotation(float angleDegrees, const glm::vec3& axis);

    const glm::mat4& getModelMatrix() const { return m_modelMatrix; }

    size_t meshCount() const { return m_meshes.size(); }

private:
    void rebuildModelMatrix();

    std::vector<std::unique_ptr<Mesh>> m_meshes;

    glm::vec3 m_position    { 0.0f };
    glm::vec3 m_scale       { 1.0f };
    float     m_angleDeg    { 0.0f };
    glm::vec3 m_rotationAxis{ 0.0f, 1.0f, 0.0f };
    glm::mat4 m_baseRotation{ 1.0f };  // orientation fix, identity by default

    glm::mat4 m_modelMatrix { 1.0f };
};

} // namespace Renderer