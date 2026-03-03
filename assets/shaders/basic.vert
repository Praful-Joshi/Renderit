#version 330 core

// ── Vertex inputs — match Mesh::Vertex layout and VertexAttribute locations ──
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_texCoord;
layout(location = 2) in vec3 a_normal;
layout(location = 3) in vec3 a_tangent;
layout(location = 4) in vec3 a_bitangent;

// ── Uniforms ──────────────────────────────────────────────────────────────────
uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

// ── Outputs to fragment shader ────────────────────────────────────────────────
out vec2 v_texCoord;
out vec3 v_normal;
out vec3 v_fragPos;    // world-space position for lighting
out vec3 v_tangent;
out vec3 v_bitangent;

void main() {
    vec4 worldPos = u_model * vec4(a_position, 1.0);
    gl_Position   = u_projection * u_view * worldPos;

    v_texCoord = a_texCoord;
    v_fragPos  = vec3(worldPos);

    // Normal matrix: transpose of inverse of model matrix.
    // Needed to correctly transform normals when the model has non-uniform scale.
    // For uniform scale, this equals the upper-left 3x3 of u_model.
    mat3 normalMatrix = mat3(transpose(inverse(u_model)));
    v_normal    = normalize(normalMatrix * a_normal);
    v_tangent   = normalize(normalMatrix * a_tangent);
    v_bitangent = normalize(normalMatrix * a_bitangent);
}
