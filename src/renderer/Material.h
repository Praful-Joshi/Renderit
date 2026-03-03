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
// Current maps:    diffuse, normal
// Step 8 will add: roughness, metallic, AO (PBR workflow)
class Material {
public:
    Material() = default;

    // Bind all available textures to their designated units and set
    // all uniforms the shader expects. Call before mesh->draw().
    void bind(const Shader& shader) const;
    void unbind() const;

    // ── Texture maps ─────────────────────────────────────────────────────────
    // Texture units are fixed by convention so shaders always know where
    // to find each map:
    //   unit 0 = diffuse
    //   unit 1 = normal    (Step 7)
    //   unit 2 = roughness (Step 8)
    //   unit 3 = metallic  (Step 8)
    //   unit 4 = AO        (Step 8)
    std::shared_ptr<Texture> diffuseMap;
    std::shared_ptr<Texture> normalMap;

    // ── Scalar fallbacks ──────────────────────────────────────────────────────
    // Used when no texture map is present for a given property.
    glm::vec3 diffuseColor  { 0.8f, 0.8f, 0.8f };
    glm::vec3 specularColor { 0.5f, 0.5f, 0.5f };
    float     shininess     { 32.0f };

    std::string name;
};

} // namespace Renderer