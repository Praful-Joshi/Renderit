#version 330 core

in vec2 v_texCoord;
in vec3 v_normal;
in vec3 v_fragPos;
in vec3 v_tangent;
in vec3 v_bitangent;

// ── Material ──────────────────────────────────────────────────────────────────
// Supports both full PBR (metallic/roughness workflow) and legacy Blinn-Phong.
// The shader picks the right path based on which maps/flags are present.
struct Material {
    // Albedo / base color (unit 0)
    sampler2D diffuseMap;
    bool      hasDiffuse;
    vec3      diffuseColor;     // fallback when no diffuse map

    // Normal map (unit 1)
    sampler2D normalMap;
    bool      hasNormal;

    // PBR maps
    sampler2D roughnessMap;     // unit 2
    bool      hasRoughness;
    float     roughness;        // fallback scalar (0=mirror, 1=fully rough)

    sampler2D metallicMap;      // unit 3
    bool      hasMetallic;
    float     metallic;         // fallback scalar (0=dielectric, 1=metal)

    sampler2D aoMap;            // unit 4
    bool      hasAO;

    // Legacy Blinn-Phong fallbacks (used when no PBR maps present)
    vec3  specularColor;
    float shininess;
};
uniform Material u_material;

// ── Point Light ───────────────────────────────────────────────────────────────
struct PointLight {
    vec3  position;
    vec3  color;
    float constant;
    float linear;
    float quadratic;
};
uniform PointLight u_light;

// ── Camera ────────────────────────────────────────────────────────────────────
uniform vec3 u_cameraPos;

out vec4 FragColor;

// ═════════════════════════════════════════════════════════════════════════════
//  PBR helper functions
// ═════════════════════════════════════════════════════════════════════════════

const float PI = 3.14159265359;

// ── D: GGX Normal Distribution Function ──────────────────────────────────────
// Estimates the fraction of microfacets aligned to H.
// High roughness → broad distribution → large soft highlight.
// Low roughness  → narrow distribution → tight sharp highlight.
float D_GGX(vec3 N, vec3 H, float roughness) {
    float a  = roughness * roughness;
    float a2 = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom  = (NdotH2 * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom);
}

// ── G: Schlick-GGX sub-function (single direction) ───────────────────────────
float G_SchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

// ── G: Smith — combine both L and V directions ────────────────────────────────
float G_Smith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return G_SchlickGGX(NdotV, roughness) * G_SchlickGGX(NdotL, roughness);
}

// ── F: Fresnel-Schlick ────────────────────────────────────────────────────────
// F0 = base reflectance at 0 degrees (dielectrics ~0.04, metals = albedo).
vec3 F_Schlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// ═════════════════════════════════════════════════════════════════════════════
//  Main
// ═════════════════════════════════════════════════════════════════════════════

void main() {

    // ── Sample material maps ──────────────────────────────────────────────────
    // Albedo textures are stored in sRGB. Convert to linear before any math.
    vec3 albedo = u_material.hasDiffuse
        ? pow(texture(u_material.diffuseMap, v_texCoord).rgb, vec3(2.2))
        : u_material.diffuseColor;

    float roughness = u_material.hasRoughness
        ? texture(u_material.roughnessMap, v_texCoord).r
        : u_material.roughness;

    float metallic = u_material.hasMetallic
        ? texture(u_material.metallicMap, v_texCoord).r
        : u_material.metallic;

    float ao = u_material.hasAO
        ? texture(u_material.aoMap, v_texCoord).r
        : 1.0;

    // ── Normal (TBN — same as Step 7) ────────────────────────────────────────
    vec3 T     = normalize(v_tangent);
    vec3 B     = normalize(v_bitangent);
    vec3 Ngeom = normalize(v_normal);
    mat3 TBN   = mat3(T, B, Ngeom);

    vec3 N;
    if (u_material.hasNormal) {
        vec3 nTangent = texture(u_material.normalMap, v_texCoord).rgb;
        nTangent = normalize(nTangent * 2.0 - 1.0);
        N = normalize(TBN * nTangent);
    } else {
        N = Ngeom;
    }

    // ── View and light vectors ────────────────────────────────────────────────
    vec3 V = normalize(u_cameraPos - v_fragPos);
    vec3 L = normalize(u_light.position - v_fragPos);
    vec3 H = normalize(V + L);

    // ── Attenuation ───────────────────────────────────────────────────────────
    float d           = length(u_light.position - v_fragPos);
    float attenuation = 1.0 / (u_light.constant
                              + u_light.linear    * d
                              + u_light.quadratic * d * d);
    vec3 radiance = u_light.color * attenuation;

    // ── PBR or Blinn-Phong? ───────────────────────────────────────────────────
    // Fall back to Blinn-Phong for models without metallic/roughness maps
    // so the cottage and character model continue to look correct.
    bool isPBR = u_material.hasRoughness || u_material.hasMetallic;

    vec3 result;

    if (isPBR) {
        // ── Cook-Torrance BRDF ────────────────────────────────────────────────

        // F0: base reflectance at normal incidence.
        //   Dielectrics ≈ 0.04 for almost all non-metals.
        //   Metals: F0 is the albedo color (copper, gold etc.).
        vec3 F0 = mix(vec3(0.04), albedo, metallic);

        float D = D_GGX(N, H, roughness);
        float G = G_Smith(N, V, L, roughness);
        vec3  F = F_Schlick(max(dot(H, V), 0.0), F0);

        float NdotL = max(dot(N, L), 0.0);
        float NdotV = max(dot(N, V), 0.0);

        // Cook-Torrance specular lobe
        vec3 specular = (D * G * F) / (4.0 * NdotV * NdotL + 0.0001);

        // Energy conservation:
        //   ks = Fresnel (specularly reflected fraction)
        //   kd = remainder that enters the surface and diffuses out
        //   Metals have zero diffuse (all energy stays in specular).
        vec3 ks = F;
        vec3 kd = (vec3(1.0) - ks) * (1.0 - metallic);

        vec3 Lo = (kd * albedo / PI + specular) * radiance * NdotL;

        // Ambient — simple constant. Step 10 replaces this with IBL.
        vec3 ambient = vec3(0.03) * albedo * ao;

        result = ambient + Lo;

        // ── Tone mapping + gamma correction ───────────────────────────────────
        // PBR math runs in HDR linear space. Map to LDR sRGB for display.
        result = result / (result + vec3(1.0));   // Reinhard tone map
        result = pow(result, vec3(1.0 / 2.2));    // linear → sRGB

    } else {
        // ── Legacy Blinn-Phong (cottage, girl model, etc.) ────────────────────
        float ambientStrength = 0.12;
        vec3  ambient  = ambientStrength * u_light.color * albedo;
        float diff     = max(dot(N, L), 0.0);
        vec3  diffuse  = diff * u_light.color * albedo;
        float spec     = pow(max(dot(N, H), 0.0), u_material.shininess);
        vec3  specular = spec * u_light.color * u_material.specularColor;
        result = ambient + (diffuse + specular) * attenuation;
    }

    FragColor = vec4(result, 1.0);
}
