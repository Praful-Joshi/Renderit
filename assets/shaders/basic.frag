#version 330 core

in vec2 v_texCoord;
in vec3 v_normal;
in vec3 v_fragPos;
in vec3 v_tangent;
in vec3 v_bitangent;

// ── Material ──────────────────────────────────────────────────────────────────
struct Material {
    sampler2D diffuseMap;
    sampler2D normalMap;

    bool  hasDiffuse;
    bool  hasNormal;

    vec3  diffuseColor;   // fallback when no diffuse map
    vec3  specularColor;  // how much specular light this surface reflects
    float shininess;      // tightness of specular highlight (8=soft, 256=sharp)
};
uniform Material u_material;

// ── Point Light ───────────────────────────────────────────────────────────────
struct PointLight {
    vec3  position;   // world space position
    vec3  color;      // RGB color and intensity combined

    // Attenuation coefficients — control how fast light fades with distance.
    // Formula: 1.0 / (constant + linear*d + quadratic*d²)
    // These defaults give a light radius of roughly 50 units.
    float constant;   // always 1.0 — prevents division by zero at d=0
    float linear;     // linear falloff term
    float quadratic;  // quadratic falloff term — makes it fall off fast at range
};
uniform PointLight u_light;

// ── Camera ────────────────────────────────────────────────────────────────────
uniform vec3 u_cameraPos;  // world space — needed to compute view direction

out vec4 FragColor;

void main() {

    // ── Surface color ─────────────────────────────────────────────────────────
    // Get the base color from the diffuse texture or the fallback color.
    vec3 surfaceColor = u_material.hasDiffuse
        ? texture(u_material.diffuseMap, v_texCoord).rgb
        : u_material.diffuseColor;

    // ── Vectors ───────────────────────────────────────────────────────────────
    // All vectors must be unit length (normalized) for dot products to give
    // correct cosine values. Interpolation across a triangle can un-normalize
    // them slightly, so we renormalize here in the fragment shader.
    vec3 N = normalize(v_normal);

    // Direction FROM this fragment TO the light source.
    // Not the other way around — we want the angle between the surface
    // normal and the incoming light direction.
    vec3 L = normalize(u_light.position - v_fragPos);

    // Direction FROM this fragment TO the camera.
    vec3 V = normalize(u_cameraPos - v_fragPos);

    // Blinn-Phong halfway vector — bisects the angle between L and V.
    // When H aligns with N, the surface is perfectly oriented to reflect
    // the light directly toward the viewer → maximum specular highlight.
    vec3 H = normalize(L + V);

    // ── Attenuation ───────────────────────────────────────────────────────────
    // Light intensity falls off with distance. Without this, a point light
    // would illuminate objects equally at 1 unit and 1000 units away.
    float d           = length(u_light.position - v_fragPos);
    float attenuation = 1.0 / (u_light.constant
                              + u_light.linear    * d
                              + u_light.quadratic * d * d);

    // ── Ambient ───────────────────────────────────────────────────────────────
    // Constant baseline — prevents surfaces facing away from the light
    // from being completely black. Represents indirect/bounced light.
    float ambientStrength = 0.12;
    vec3  ambient = ambientStrength * u_light.color * surfaceColor;

    // ── Diffuse ───────────────────────────────────────────────────────────────
    // Lambert's cosine law: brightness proportional to cos(angle between N and L).
    // max(..., 0.0) clamps negative values — surface facing away contributes nothing.
    float diff    = max(dot(N, L), 0.0);
    vec3  diffuse = diff * u_light.color * surfaceColor;

    // ── Specular ─────────────────────────────────────────────────────────────
    // pow() makes the falloff sharp — high shininess = tight highlight (shiny)
    //                                  low shininess  = broad highlight (matte)
    // u_material.specularColor controls how much the surface reflects specular
    // light — metals reflect a lot, rough wood reflects very little.
    float spec     = pow(max(dot(N, H), 0.0), u_material.shininess);
    vec3  specular = spec * u_light.color * u_material.specularColor;

    // ── Combine ───────────────────────────────────────────────────────────────
    // Apply attenuation to diffuse and specular but NOT ambient.
    // Ambient is scene-wide indirect light — it doesn't come from a specific
    // source so it doesn't attenuate with distance.
    vec3 result = ambient + (diffuse + specular) * attenuation;

    FragColor = vec4(result, 1.0);
}
