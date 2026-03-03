#include "Mesh.h"
#include <glad/glad.h>

namespace Renderer {

Mesh::Mesh(const std::vector<Vertex>& vertices,
           const std::vector<uint32_t>& indices)
{
    m_vertexCount = vertices.size();
    m_indexCount  = indices.size();
    setupBuffer(vertices, indices);
}

Mesh::Mesh(const std::vector<Vertex>& vertices,
           const std::vector<uint32_t>& indices,
           std::shared_ptr<Texture> texture)
    : m_texture(std::move(texture))
{
    m_vertexCount = vertices.size();
    m_indexCount  = indices.size();
    setupBuffer(vertices, indices);
}

void Mesh::setupBuffer(const std::vector<Vertex>& vertices,
                       const std::vector<uint32_t>& indices)
{
    // Memory layout of one Vertex (stride = 32 bytes):
    //
    //  Offset:  0              12         20              32
    //           |── pos (vec3)──|── uv ────|── normal ─────|
    //           | x    y    z  | u    v   | nx   ny   nz  |
    //           |    12 bytes  |  8 bytes |    12 bytes   |

    std::vector<float> flatData;
    flatData.reserve(vertices.size() * 8);

    for (const auto& v : vertices) {
        flatData.push_back(v.position.x);
        flatData.push_back(v.position.y);
        flatData.push_back(v.position.z);
        flatData.push_back(v.texCoord.x);
        flatData.push_back(v.texCoord.y);
        flatData.push_back(v.normal.x);
        flatData.push_back(v.normal.y);
        flatData.push_back(v.normal.z);
    }

    std::vector<VertexAttribute> attributes = {
        { 0, 3,  0 },   // a_position : vec3 at offset 0
        { 1, 2, 12 },   // a_texCoord : vec2 at offset 12
        { 2, 3, 20 },   // a_normal   : vec3 at offset 20
    };

    m_buffer.uploadVertices(flatData, attributes, sizeof(Vertex));
    m_buffer.uploadIndices(indices);
}

void Mesh::draw(const Shader& shader) const {
    // If this mesh has a texture, bind it to unit 0 and tell the
    // shader's sampler uniform which unit to read from.
    if (m_texture) {
        m_texture->bind(0);
        shader.setInt("u_diffuse", 0);    // sampler2D u_diffuse reads unit 0
        shader.setBool("u_hasTexture", true);
    } else {
        shader.setBool("u_hasTexture", false);
    }

    m_buffer.bind();
    glDrawElements(GL_TRIANGLES,
                   static_cast<GLsizei>(m_indexCount),
                   GL_UNSIGNED_INT,
                   0);
    m_buffer.unbind();

    if (m_texture)
        m_texture->unbind();
}

} // namespace Renderer