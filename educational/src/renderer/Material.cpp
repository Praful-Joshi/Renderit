#include "Material.h"

namespace Renderer {

void Material::bind(const Shader& shader) const {

    // ── Albedo / diffuse map (unit 0) ─────────────────────────────────────────
    if (diffuseMap) {
        diffuseMap->bind(0);
        shader.setInt ("u_material.diffuseMap",  0);
        shader.setBool("u_material.hasDiffuse",  true);
    } else {
        shader.setBool("u_material.hasDiffuse",  false);
        shader.setVec3("u_material.diffuseColor", diffuseColor);
    }

    // ── Normal map (unit 1) ───────────────────────────────────────────────────
    if (normalMap) {
        normalMap->bind(1);
        shader.setInt ("u_material.normalMap",  1);
        shader.setBool("u_material.hasNormal",  true);
    } else {
        shader.setBool("u_material.hasNormal",  false);
    }

    // ── Roughness map (unit 2) ────────────────────────────────────────────────
    if (roughnessMap) {
        roughnessMap->bind(2);
        shader.setInt ("u_material.roughnessMap",  2);
        shader.setBool("u_material.hasRoughness",  true);
    } else {
        shader.setBool("u_material.hasRoughness",  false);
        shader.setFloat("u_material.roughness",    roughness);
    }

    // ── Metallic map (unit 3) ─────────────────────────────────────────────────
    if (metallicMap) {
        metallicMap->bind(3);
        shader.setInt ("u_material.metallicMap",  3);
        shader.setBool("u_material.hasMetallic",  true);
    } else {
        shader.setBool("u_material.hasMetallic",  false);
        shader.setFloat("u_material.metallic",    metallic);
    }

    // ── AO map (unit 4) ───────────────────────────────────────────────────────
    if (aoMap) {
        aoMap->bind(4);
        shader.setInt ("u_material.aoMap",  4);
        shader.setBool("u_material.hasAO",  true);
    } else {
        shader.setBool("u_material.hasAO",  false);
    }

    // ── Legacy Blinn-Phong scalars ────────────────────────────────────────────
    // Still set so the shader can fall back gracefully for non-PBR models.
    shader.setVec3 ("u_material.specularColor", specularColor);
    shader.setFloat("u_material.shininess",     shininess);
}

void Material::unbind() const {
    if (diffuseMap)   diffuseMap->unbind();
    if (normalMap)    normalMap->unbind();
    if (roughnessMap) roughnessMap->unbind();
    if (metallicMap)  metallicMap->unbind();
    if (aoMap)        aoMap->unbind();
}

} // namespace Renderer