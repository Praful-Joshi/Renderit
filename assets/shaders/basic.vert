#version 330 core

// ── Inputs ────────────────────────────────────────────────────────────────────
// 'layout(location = 0)' matches VertexAttribute.location = 0 in Buffer setup
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_color;

// ── Uniforms ──────────────────────────────────────────────────────────────────
// Set once per draw call from the CPU via shader.setMat4(...)
uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

// ── Outputs to fragment shader ────────────────────────────────────────────────
out vec3 v_color;  // 'v_' prefix = varying — interpolated across the triangle

void main() {
    // The MVP transform — every 3D renderer in history does this one line
    // Read right to left: model space → world → view → clip
    gl_Position = u_projection * u_view * u_model * vec4(a_position, 1.0);

    // Pass color through to the fragment shader
    // The GPU will interpolate this smoothly across the triangle surface
    v_color = a_color;
}
