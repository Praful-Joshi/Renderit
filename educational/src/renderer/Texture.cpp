#include "Texture.h"

// STB_IMAGE_IMPLEMENTATION must be defined in exactly one .cpp file.
// This tells the header to include the full function definitions here.
// Every other file that needs stb_image just includes the header without
// the define — otherwise you'd get duplicate symbol linker errors.
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <iostream>
#include <stdexcept>

namespace Renderer {

Texture::Texture(const std::string& path) : m_path(path) {

    // stb_image loads images with (0,0) at the top-left.
    // OpenGL expects (0,0) at the bottom-left.
    // This one call flips all images on load so UVs work correctly.
    stbi_set_flip_vertically_on_load(true);

    // Decode the image preserving its original channel count.
    // Forcing 4 channels (RGBA) on everything corrupts single-channel maps
    // (roughness, AO) and can shift the blue channel of normal maps.
    // We pass 0 so stb_image returns exactly what's in the file, then
    // pick the matching GL format below.
    unsigned char* data = stbi_load(path.c_str(),
                                    &m_width, &m_height, &m_channels, 0);
    if (!data) {
        throw std::runtime_error("[Texture] Failed to load: " + path +
                                 "\n  Reason: " + stbi_failure_reason());
    }

    std::cout << "[Texture] Loaded: " << path
              << " (" << m_width << "x" << m_height
              << ", " << m_channels << " channels)\n";

    // Choose GL format based on actual channel count.
    // internalFormat = how the GPU stores it.
    // dataFormat     = how the CPU-side bytes are laid out.
    GLenum internalFormat, dataFormat;
    switch (m_channels) {
        case 1:  internalFormat = GL_R8;    dataFormat = GL_RED;  break;
        case 2:  internalFormat = GL_RG8;   dataFormat = GL_RG;   break;
        case 3:  internalFormat = GL_RGB8;  dataFormat = GL_RGB;  break;
        case 4:  internalFormat = GL_RGBA8; dataFormat = GL_RGBA; break;
        default: internalFormat = GL_RGBA8; dataFormat = GL_RGBA; break;
    }

    // ── Generate GPU texture object ───────────────────────────────────────────
    glGenTextures(1, &m_textureID);
    glBindTexture(GL_TEXTURE_2D, m_textureID);

    // For single-channel textures (roughness, AO, metallic) the default swizzle
    // reads R into R only, leaving G/B as 0. We want to sample it as .r in GLSL
    // which works fine — but set swizzle so all channels read from R, making
    // texture().rgb work correctly too if ever needed.
    if (m_channels == 1) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
    }

    // ── Wrapping mode ─────────────────────────────────────────────────────────
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // ── Filtering mode ────────────────────────────────────────────────────────
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // ── Upload pixel data to GPU ──────────────────────────────────────────────
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 internalFormat,
                 m_width, m_height,
                 0,
                 dataFormat,
                 GL_UNSIGNED_BYTE,
                 data);

    // ── Generate mipmap chain ─────────────────────────────────────────────────
    // Automatically creates 512x512, 256x256, 128x128... down to 1x1.
    // Must be called AFTER glTexImage2D so the base level exists.
    glGenerateMipmap(GL_TEXTURE_2D);

    // ── Free CPU memory ───────────────────────────────────────────────────────
    // GPU now has its own copy. The CPU-side bytes are no longer needed.
    stbi_image_free(data);

    glBindTexture(GL_TEXTURE_2D, 0);
}

Texture::~Texture() {
    if (m_textureID)
        glDeleteTextures(1, &m_textureID);
}

Texture::Texture(Texture&& other) noexcept
    : m_textureID(other.m_textureID),
      m_width(other.m_width),
      m_height(other.m_height),
      m_channels(other.m_channels),
      m_path(std::move(other.m_path))
{
    other.m_textureID = 0;
}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        if (m_textureID) glDeleteTextures(1, &m_textureID);
        m_textureID       = other.m_textureID;
        m_width           = other.m_width;
        m_height          = other.m_height;
        m_channels        = other.m_channels;
        m_path            = std::move(other.m_path);
        other.m_textureID = 0;
    }
    return *this;
}

void Texture::bind(GLuint unit) const {
    // Activate the texture unit slot first, then bind our texture to it.
    // GL_TEXTURE0 + 1 = GL_TEXTURE1, + 2 = GL_TEXTURE2, etc.
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, m_textureID);
}

void Texture::unbind() const {
    glBindTexture(GL_TEXTURE_2D, 0);
}

} // namespace Renderer