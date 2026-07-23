#pragma once

#include "Texture.h"
#include "Shader.h"

#include <glm/glm.hpp>
#include <memory>
#include <string>

namespace Renderer {

// A Material describes how a surface looks — which texture maps it has
// and what scalar properties it carries. It knows how to bind itself
// to a shader before a draw call.
//
// Supports both legacy Blinn-Phong and full PBR (metallic/roughness workflow).
// The shader selects the right path based on which maps are present.
//
// Texture unit convention (fixed — shaders rely on these slots):
//   unit 0 = albedo/diffuse
//   unit 1 = normal
//   unit 2 = roughness
//   unit 3 = metallic
//   unit 4 = ambient occlusion (AO)
class Material {
public:
    Material() = default;

    // Bind all available textures to their designated units and set
    // all uniforms the shader expects. Call before mesh->draw().
    void bind(const Shader& shader) const;
    void unbind() const;

    // ── Texture maps ──────────────────────────────────────────────────────────
    std::shared_ptr<Texture> diffuseMap;    // unit 0 — albedo / base color
    std::shared_ptr<Texture> normalMap;     // unit 1 — tangent-space normals
    std::shared_ptr<Texture> roughnessMap;  // unit 2 — perceptual roughness
    std::shared_ptr<Texture> metallicMap;   // unit 3 — metallic mask
    std::shared_ptr<Texture> aoMap;         // unit 4 — ambient occlusion

    // ── Scalar fallbacks ──────────────────────────────────────────────────────
    // Used when the corresponding texture map is absent.
    glm::vec3 diffuseColor  { 0.8f, 0.8f, 0.8f };
    float     roughness     { 0.5f };   // 0 = mirror-smooth, 1 = fully rough
    float     metallic      { 0.0f };   // 0 = dielectric,    1 = metal

    // Legacy Blinn-Phong fallbacks (used when no PBR maps are present)
    glm::vec3 specularColor { 0.5f, 0.5f, 0.5f };
    float     shininess     { 32.0f };

    std::string name;
};

} // namespace Renderer