#version 330 core

// No output needed — OpenGL writes gl_FragDepth automatically.
// This shader exists only so we have a complete program to link.
// Some drivers require an explicit empty fragment shader for depth-only passes.

void main() {
    // Depth is written implicitly from gl_FragCoord.z.
    // No color output — the framebuffer has no color attachment.
}
