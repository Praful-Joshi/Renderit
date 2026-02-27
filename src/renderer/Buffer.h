#pragma once

#include <glad/glad.h>
#include <vector>
#include <cstdint>

namespace Renderer {

// Describes one attribute in the vertex layout.
// e.g. position: { location=0, count=3, offset=0 }
//      texCoord: { location=1, count=2, offset=12 }
struct VertexAttribute {
    GLuint  location;   // matches 'layout(location = N)' in the vertex shader
    GLint   count;      // number of floats (2 for vec2, 3 for vec3, etc.)
    GLsizei offset;     // byte offset from the start of one vertex
};

class Buffer {
public:
    Buffer();
    ~Buffer();

    // Non-copyable — owns GPU handles
    Buffer(const Buffer&)            = delete;
    Buffer& operator=(const Buffer&) = delete;

    // Movable
    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;

    // Upload vertex data and describe its layout.
    // vertices: flat array of floats [ x,y,z, u,v,  x,y,z, u,v, ... ]
    // attributes: describes each field within one vertex
    // stride: total byte size of one complete vertex
    void uploadVertices(const std::vector<float>& vertices,
                        const std::vector<VertexAttribute>& attributes,
                        GLsizei stride);

    // Optional: upload index data for indexed drawing
    void uploadIndices(const std::vector<uint32_t>& indices);

    void bind()   const;
    void unbind() const;

    bool     hasIndices()  const { return m_ebo != 0; }
    GLsizei  indexCount()  const { return m_indexCount; }
    GLsizei  vertexCount() const { return m_vertexCount; }

private:
    GLuint  m_vao         = 0;
    GLuint  m_vbo         = 0;
    GLuint  m_ebo         = 0;
    GLsizei m_indexCount  = 0;
    GLsizei m_vertexCount = 0;
};

} // namespace Renderer