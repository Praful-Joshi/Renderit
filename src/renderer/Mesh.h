#pragma once

#include "Texture.h"  // must come before Buffer.h and Shader.h
#include "Buffer.h"
#include "Shader.h"

#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <memory>

namespace Renderer {

// One vertex — the interleaved layout uploaded to the GPU.
struct Vertex {
    glm::vec3 position;
    glm::vec2 texCoord;
    glm::vec3 normal;
};

// A Mesh is one drawable unit of geometry with its own texture.
// It owns its GPU buffers and knows how to draw itself.
class Mesh {
public:
    // Construct with geometry only (no texture — renders with whatever
    // the shader does by default, e.g. normal visualization)
    Mesh(const std::vector<Vertex>& vertices,
         const std::vector<uint32_t>& indices);

    // Construct with geometry and a texture
    Mesh(const std::vector<Vertex>& vertices,
         const std::vector<uint32_t>& indices,
         std::shared_ptr<Texture> texture);

    Mesh(const Mesh&)            = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&&)                 = default;
    Mesh& operator=(Mesh&&)      = default;

    // Bind texture (if any) to unit 0, issue draw call.
    // Shader must already be bound before calling draw().
    void draw(const Shader& shader) const;

    void setTexture(std::shared_ptr<Texture> texture) { m_texture = texture; }
    bool hasTexture() const { return m_texture != nullptr; }

    size_t vertexCount() const { return m_vertexCount; }
    size_t indexCount()  const { return m_indexCount; }

private:
    void setupBuffer(const std::vector<Vertex>& vertices,
                     const std::vector<uint32_t>& indices);

    Buffer                   m_buffer;
    std::shared_ptr<Texture> m_texture;   // shared_ptr: multiple meshes can share one texture
    size_t m_vertexCount = 0;
    size_t m_indexCount  = 0;
};

} // namespace Renderer