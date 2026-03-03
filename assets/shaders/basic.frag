#version 330 core

in vec2 v_texCoord;
in vec3 v_normal;
in vec3 v_fragPos;

// The diffuse texture — set to texture unit 0 from Mesh::draw()
uniform sampler2D u_diffuse;

// Whether this mesh has a texture.
// Meshes without one (ground plane, lights) fall back to normal colors
// so we can still see them clearly.
uniform bool u_hasTexture;

out vec4 FragColor;

void main() {
    if (u_hasTexture) {
        // Sample the texture at this pixel's interpolated UV coordinate.
        // texture() is a GLSL built-in — it handles all the filtering
        // and mipmap selection we configured in Texture.cpp.
        FragColor = texture(u_diffuse, v_texCoord);
    } else {
        // Fallback: visualize normals as color for untextured meshes
        vec3 normalColor = normalize(v_normal) * 0.5 + 0.5;
        FragColor = vec4(normalColor, 1.0);
    }
}
