#pragma once

#include <cstdint>
#include <glad/glad.h>
#include <vector>

namespace Renderer
{

/**
 * @struct VertexAttribute
 * @brief Describes a single vertex attribute layout entry.
 *
 * Maps directly to:
 *   layout(location = X) in GLSL.
 */
struct VertexAttribute
{
    GLuint  location; ///< Attribute location in shader
    GLint   count;    ///< Number of components (e.g., 3 for vec3)
    GLsizei offset;   ///< Byte offset from start of vertex
};

/**
 * @class Buffer
 * @brief RAII wrapper for VAO + VBO (+ optional EBO).
 *
 * @details
 * Responsible for:
 *  - Allocating OpenGL vertex buffers
 *  - Uploading vertex data
 *  - Defining attribute layout
 *  - Optional indexed drawing support
 *
 * Ownership:
 *  - Owns VAO, VBO, optional EBO
 *  - Non-copyable
 *  - Movable
 */
class Buffer
{
  public:
    Buffer();
    ~Buffer();

    Buffer(const Buffer&)            = delete;
    Buffer& operator=(const Buffer&) = delete;

    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;

    /**
     * @brief Uploads vertex data and configures layout.
     *
     * @param vertices Flat float array (interleaved)
     * @param attributes Vertex attribute descriptions
     * @param stride Size of one vertex in bytes
     */
    void uploadVertices(const std::vector<float>&           vertices,
                        const std::vector<VertexAttribute>& attributes, GLsizei stride);

    /**
     * @brief Uploads index data for indexed drawing.
     *
     * @param indices Index buffer data
     *
     * @throws std::runtime_error if called before uploadVertices.
     */
    void uploadIndices(const std::vector<uint32_t>& indices);

    void bind() const;
    void unbind() const;

    bool hasIndices() const
    {
        return m_ebo != 0;
    }
    GLsizei indexCount() const
    {
        return m_indexCount;
    }
    GLsizei vertexCount() const
    {
        return m_vertexCount;
    }

  private:
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_ebo = 0;

    GLsizei m_indexCount  = 0;
    GLsizei m_vertexCount = 0;
};

} // namespace Renderer