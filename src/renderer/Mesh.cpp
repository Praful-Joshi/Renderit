#include "Mesh.h"
#include <glad/glad.h>

namespace Renderer
{

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
{
    m_vertexCount = vertices.size();
    m_indexCount  = indices.size();
    setupBuffer(vertices, indices);
}

void Mesh::setupBuffer(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
{
    // Convert our typed Vertex structs into a flat float array for upload.
    // The GPU just sees bytes — we describe the layout via VertexAttributes.
    //
    // Memory layout of one Vertex (stride = 32 bytes):
    //
    //  Offset 0              12         20              32
    //  |── position (vec3) ──|── uv ────|── normal ─────|
    //  | x    y    z         | u    v   | nx   ny   nz  |
    //  | 4    4    4  bytes  | 4    4   |  4    4    4  |

    std::vector<float> flatData;
    flatData.reserve(vertices.size() * 8); // 8 floats per vertex

    for (const auto& v : vertices)
    {
        flatData.push_back(v.position.x);
        flatData.push_back(v.position.y);
        flatData.push_back(v.position.z);
        flatData.push_back(v.texCoord.x);
        flatData.push_back(v.texCoord.y);
        flatData.push_back(v.normal.x);
        flatData.push_back(v.normal.y);
        flatData.push_back(v.normal.z);
    }

    // Describe the layout:
    //   location 0 = position: 3 floats, offset 0
    //   location 1 = texCoord: 2 floats, offset 12  (3 × 4 bytes)
    //   location 2 = normal:   3 floats, offset 20  (3+2) × 4 bytes)
    std::vector<VertexAttribute> attributes = {
        {0, 3, 0},  // a_position
        {1, 2, 12}, // a_texCoord
        {2, 3, 20}, // a_normal
    };

    GLsizei stride = sizeof(Vertex); // 32 bytes

    m_buffer.uploadVertices(flatData, attributes, stride);
    m_buffer.uploadIndices(indices);
}

void Mesh::draw(const Shader& shader) const
{
    // Shader is already bound by the caller — Mesh just binds its geometry
    // and issues the draw call. This keeps draw() fast and simple.
    (void)shader; // reserved for texture binding in Step 3

    m_buffer.bind();
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indexCount), GL_UNSIGNED_INT, 0);
    m_buffer.unbind();
}

} // namespace Renderer