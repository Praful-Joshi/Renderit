#include "Buffer.h"
#include <stdexcept>

namespace Renderer
{

Buffer::Buffer()
{
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
}

Buffer::~Buffer()
{
    if (m_ebo)
        glDeleteBuffers(1, &m_ebo);
    if (m_vbo)
        glDeleteBuffers(1, &m_vbo);
    if (m_vao)
        glDeleteVertexArrays(1, &m_vao);
}

Buffer::Buffer(Buffer&& other) noexcept
    : m_vao(other.m_vao), m_vbo(other.m_vbo), m_ebo(other.m_ebo), m_indexCount(other.m_indexCount),
      m_vertexCount(other.m_vertexCount)
{
    other.m_vao = other.m_vbo = other.m_ebo = 0;
}

Buffer& Buffer::operator=(Buffer&& other) noexcept
{
    if (this != &other)
    {
        if (m_ebo)
            glDeleteBuffers(1, &m_ebo);
        if (m_vbo)
            glDeleteBuffers(1, &m_vbo);
        if (m_vao)
            glDeleteVertexArrays(1, &m_vao);

        m_vao         = other.m_vao;
        m_vbo         = other.m_vbo;
        m_ebo         = other.m_ebo;
        m_indexCount  = other.m_indexCount;
        m_vertexCount = other.m_vertexCount;
        other.m_vao = other.m_vbo = other.m_ebo = 0;
    }
    return *this;
}

void Buffer::uploadVertices(const std::vector<float>&           vertices,
                            const std::vector<VertexAttribute>& attributes, GLsizei stride)
{
    // The VAO records all the state set up below.
    // After unbinding, re-binding the VAO restores all of it.
    glBindVertexArray(m_vao);

    // Upload raw float data to GPU memory
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(),
                 GL_STATIC_DRAW); // hint: data uploaded once, drawn many times

    // Describe the layout of that data — one call per attribute
    for (const auto& attr : attributes)
    {
        glVertexAttribPointer(attr.location, // which shader input slot
                              attr.count,    // how many floats
                              GL_FLOAT,      // data type
                              GL_FALSE,      // don't normalize
                              stride,        // bytes between consecutive vertices
                              reinterpret_cast<void*>(static_cast<uintptr_t>(attr.offset)));
        glEnableVertexAttribArray(attr.location);
    }

    m_vertexCount = static_cast<GLsizei>(vertices.size() * sizeof(float)) / stride;

    glBindVertexArray(0);             // unbind — VAO has recorded everything
    glBindBuffer(GL_ARRAY_BUFFER, 0); // unbind - VBO's data is now in GPU
}

void Buffer::uploadIndices(const std::vector<uint32_t>& indices)
{
    if (!m_vao)
        throw std::runtime_error("[Buffer] Call uploadVertices before uploadIndices");

    glBindVertexArray(m_vao);

    if (!m_ebo)
        glGenBuffers(1, &m_ebo);

    // EBO binding is recorded by the VAO — don't unbind EBO before VAO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(),
                 GL_STATIC_DRAW);

    m_indexCount = static_cast<GLsizei>(indices.size());

    glBindVertexArray(0);
}

void Buffer::bind() const
{
    glBindVertexArray(m_vao);
}
void Buffer::unbind() const
{
    glBindVertexArray(0);
}

} // namespace Renderer