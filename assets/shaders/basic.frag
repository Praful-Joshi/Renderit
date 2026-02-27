#version 330 core

// ── Input from vertex shader ───────────────────────────────────────────────────
// Same name and type as the vertex shader's 'out' — GPU interpolates between vertices
in vec3 v_color;

// ── Output ────────────────────────────────────────────────────────────────────
// Final RGBA color written to the framebuffer for this pixel
out vec4 FragColor;

void main() {
    // 1.0 alpha = fully opaque
    FragColor = vec4(v_color, 1.0);
}
