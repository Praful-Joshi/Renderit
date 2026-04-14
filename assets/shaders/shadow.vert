#version 330 core

layout(location = 0) in vec3 a_position;

// Combined light-view * light-projection matrix.
// Transforms each vertex into the light's clip space so the GPU writes
// the correct depth value into the shadow map framebuffer.
uniform mat4 u_lightSpaceMatrix;
uniform mat4 u_model;

void main() {
    gl_Position = u_lightSpaceMatrix * u_model * vec4(a_position, 1.0);
}
