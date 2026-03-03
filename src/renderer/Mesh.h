#pragma once

#include "Buffer.h"
#include "Shader.h"
#include "Material.h"

#include <glm/glm.hpp>
#include <vector>
#include <memory>

namespace Renderer {

// One vertex — interleaved layout in GPU memory.
// Tangent and bitangent are included now, computed by Assimp,
// and will be used in Step 7 for normal mapping.
struct Vertex {
    glm::vec3 position;
    glm::vec2 texCoord;
    glm::vec3 normal;
    glm::vec3 tangent;    // points along U direction of the texture
    glm::vec3 bitangent;  // points along V direction of the texture
};

// A Mesh is one drawable unit: geometry + material.
// Assimp may produce many Meshes per model file, one per material group.
class Mesh {
public:
    Mesh(const std::vector<Vertex>&   vertices,
         const std::vector<uint32_t>& indices,
         std::shared_ptr<Material>    material);

    Mesh(const Mesh&)            = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&&)                 = default;
    Mesh& operator=(Mesh&&)      = default;

    // Binds material, issues draw call. Shader must be bound by caller.
    void draw(const Shader& shader) const;

    const Material* getMaterial() const { return m_material.get(); }
    size_t vertexCount() const { return m_vertexCount; }
    size_t indexCount()  const { return m_indexCount; }

private:
    void setupBuffer(const std::vector<Vertex>&   vertices,
                     const std::vector<uint32_t>& indices);

    Buffer                    m_buffer;
    std::shared_ptr<Material> m_material;
    size_t m_vertexCount = 0;
    size_t m_indexCount  = 0;
};

} // namespace Renderer