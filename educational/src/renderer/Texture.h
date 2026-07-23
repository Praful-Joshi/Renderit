#pragma once

#include <glad/glad.h>
#include <string>

namespace Renderer {

class Texture {
public:
    // Load an image from disk and upload to GPU.
    // Throws std::runtime_error if the file cannot be loaded.
    explicit Texture(const std::string& path);
    ~Texture();

    // Non-copyable — owns a GPU texture handle
    Texture(const Texture&)            = delete;
    Texture& operator=(const Texture&) = delete;

    // Movable
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    // Bind this texture to a specific texture unit slot.
    // unit=0 → GL_TEXTURE0, unit=1 → GL_TEXTURE1, etc.
    // Call this before the draw call that needs this texture.
    void bind(GLuint unit = 0) const;
    void unbind() const;

    GLuint      getID()     const { return m_textureID; }
    int         getWidth()  const { return m_width; }
    int         getHeight() const { return m_height; }
    const std::string& getPath() const { return m_path; }

private:
    GLuint      m_textureID = 0;
    int         m_width     = 0;
    int         m_height    = 0;
    int         m_channels  = 0;
    std::string m_path;
};

} // namespace Renderer