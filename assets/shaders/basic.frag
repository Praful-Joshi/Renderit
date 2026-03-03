#version 330 core

in vec2 v_texCoord;
in vec3 v_normal;
in vec3 v_fragPos;
in vec3 v_tangent;
in vec3 v_bitangent;

// ── Material struct ───────────────────────────────────────────────────────────
// Mirrors Material.cpp's bind() calls exactly.
// Texture unit assignments are fixed by convention:
//   unit 0 = diffuse map
//   unit 1 = normal map  (used in Step 7)
struct Material {
    sampler2D diffuseMap;
    sampler2D normalMap;

    bool  hasDiffuse;
    bool  hasNormal;

    vec3  diffuseColor;
    vec3  specularColor;
    float shininess;
};
uniform Material u_material;

out vec4 FragColor;

void main() {
    // ── Base color ────────────────────────────────────────────────────────────
    vec3 color;
    if (u_material.hasDiffuse) {
        color = texture(u_material.diffuseMap, v_texCoord).rgb;
    } else {
        // No texture — use normal visualization as placeholder.
        // Will be replaced with u_material.diffuseColor in Step 5 lighting.
        color = normalize(v_normal) * 0.5 + 0.5;
    }

    FragColor = vec4(color, 1.0);

    // Note: v_tangent and v_bitangent are passed through but unused until
    // Step 7 (normal mapping). They're here so the vertex shader outputs
    // match and the pipeline is already wired correctly.
}
