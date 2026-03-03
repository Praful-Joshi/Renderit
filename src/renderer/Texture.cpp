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

    // Decode the image into a CPU-side byte array.
    // stbi_load returns: pointer to RGBA bytes, and fills width/height/channels.
    // The final '4' requests 4 channels (RGBA) regardless of source format.
    unsigned char* data = stbi_load(path.c_str(),
                                    &m_width, &m_height, &m_channels, 4);
    if (!data) {
        throw std::runtime_error("[Texture] Failed to load: " + path +
                                 "\n  Reason: " + stbi_failure_reason());
    }

    std::cout << "[Texture] Loaded: " << path
              << " (" << m_width << "x" << m_height
              << ", " << m_channels << " channels)\n";

    // ── Generate GPU texture object ───────────────────────────────────────────
    glGenTextures(1, &m_textureID);
    glBindTexture(GL_TEXTURE_2D, m_textureID);

    // ── Wrapping mode ─────────────────────────────────────────────────────────
    // GL_REPEAT: UVs outside [0,1] tile the texture.
    // Good default for surfaces with repeating detail (bricks, wood grain).
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);  // U axis
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);  // V axis

    // ── Filtering mode ────────────────────────────────────────────────────────
    // When the texture is minified (far away): use mipmaps + linear blend
    // between two adjacent mip levels (trilinear filtering).
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    // When the texture is magnified (close up): bilinear interpolation.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // ── Upload pixel data to GPU ──────────────────────────────────────────────
    // Args: target, mip level, internal format, width, height,
    //       border (always 0), source format, data type, pixel data pointer
    glTexImage2D(GL_TEXTURE_2D,     // target
                 0,                 // mip level 0 = full resolution
                 GL_RGBA,           // internal format on GPU
                 m_width, m_height, // dimensions
                 0,                 // border — must be 0
                 GL_RGBA,           // format of the CPU-side data
                 GL_UNSIGNED_BYTE,  // each channel is one byte (0-255)
                 data);             // the actual pixel bytes

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