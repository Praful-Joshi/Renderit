#include "Mesh.h"
#include <glad/glad.h>

namespace Renderer {

Mesh::Mesh(const std::vector<Vertex>&   vertices,
           const std::vector<uint32_t>& indices,
           std::shared_ptr<Material>    material)
    : m_material(std::move(material))
{
    m_vertexCount = vertices.size();
    m_indexCount  = indices.size();
    setupBuffer(vertices, indices);
}

void Mesh::setupBuffer(const std::vector<Vertex>&   vertices,
                       const std::vector<uint32_t>& indices)
{
    // Memory layout of one Vertex (stride = 56 bytes):
    //
    // Offset  0: position  (vec3 = 12 bytes)
    // Offset 12: texCoord  (vec2 =  8 bytes)
    // Offset 20: normal    (vec3 = 12 bytes)
    // Offset 32: tangent   (vec3 = 12 bytes)
    // Offset 44: bitangent (vec3 = 12 bytes)
    // Total: 56 bytes

    std::vector<float> flatData;
    flatData.reserve(vertices.size() * 14); // 14 floats per vertex

    for (const auto& v : vertices) {
        flatData.push_back(v.position.x);
        flatData.push_back(v.position.y);
        flatData.push_back(v.position.z);
        flatData.push_back(v.texCoord.x);
        flatData.push_back(v.texCoord.y);
        flatData.push_back(v.normal.x);
        flatData.push_back(v.normal.y);
        flatData.push_back(v.normal.z);
        flatData.push_back(v.tangent.x);
        flatData.push_back(v.tangent.y);
        flatData.push_back(v.tangent.z);
        flatData.push_back(v.bitangent.x);
        flatData.push_back(v.bitangent.y);
        flatData.push_back(v.bitangent.z);
    }

    // Location binding matches the vertex shader's layout(location = N)
    std::vector<VertexAttribute> attributes = {
        { 0,  3,  0 },   // a_position  : vec3 at offset 0
        { 1,  2, 12 },   // a_texCoord  : vec2 at offset 12
        { 2,  3, 20 },   // a_normal    : vec3 at offset 20
        { 3,  3, 32 },   // a_tangent   : vec3 at offset 32
        { 4,  3, 44 },   // a_bitangent : vec3 at offset 44
    };

    m_buffer.uploadVertices(flatData, attributes, sizeof(Vertex));
    m_buffer.uploadIndices(indices);
}

void Mesh::draw(const Shader& shader) const {
    if (m_material)
        m_material->bind(shader);

    m_buffer.bind();
    glDrawElements(GL_TRIANGLES,
                   static_cast<GLsizei>(m_indexCount),
                   GL_UNSIGNED_INT,
                   0);
    m_buffer.unbind();

    if (m_material)
        m_material->unbind();
}

} // namespace Renderer