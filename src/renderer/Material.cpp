#include "Material.h"

namespace Renderer {

void Material::bind(const Shader& shader) const {

    // ── Diffuse map (unit 0) ──────────────────────────────────────────────────
    if (diffuseMap) {
        diffuseMap->bind(0);
        shader.setInt ("u_material.diffuseMap",  0);
        shader.setBool("u_material.hasDiffuse",  true);
    } else {
        shader.setBool("u_material.hasDiffuse",  false);
        shader.setVec3("u_material.diffuseColor", diffuseColor);
    }

    // ── Normal map (unit 1) ───────────────────────────────────────────────────
    // Loaded now but not used until Step 7 (TBN matrix).
    // We set the uniform regardless so the shader always has a valid value.
    if (normalMap) {
        normalMap->bind(1);
        shader.setInt ("u_material.normalMap",   1);
        shader.setBool("u_material.hasNormal",   true);
    } else {
        shader.setBool("u_material.hasNormal",   false);
    }

    // ── Scalar properties ─────────────────────────────────────────────────────
    shader.setVec3 ("u_material.specularColor", specularColor);
    shader.setFloat("u_material.shininess",     shininess);
}

void Material::unbind() const {
    if (diffuseMap) diffuseMap->unbind();
    if (normalMap)  normalMap->unbind();
}

} // namespace Renderer