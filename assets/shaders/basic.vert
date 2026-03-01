#version 330 core

// ── Vertex inputs (match Mesh::Vertex layout and VertexAttribute locations) ──
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_texCoord;
layout(location = 2) in vec3 a_normal;

// ── Uniforms ──────────────────────────────────────────────────────────────────
uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

// ── Outputs to fragment shader ────────────────────────────────────────────────
out vec2 v_texCoord;
out vec3 v_normal;
out vec3 v_fragPos;   // world-space position — needed for lighting in Step 5

void main() {
    vec4 worldPos   = u_model * vec4(a_position, 1.0);
    gl_Position     = u_projection * u_view * worldPos;

    v_texCoord = a_texCoord;
    v_fragPos  = vec3(worldPos);

    // Transform normal to world space.
    // We use the normal matrix (transpose of inverse of model matrix)
    // to handle non-uniform scaling correctly.
    // For now this works fine — we'll explain it fully in Step 5.
    v_normal = mat3(transpose(inverse(u_model))) * a_normal;
}
