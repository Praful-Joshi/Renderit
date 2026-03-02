#pragma once

#include "Buffer.h"
#include "Shader.h"

#include <glm/glm.hpp>
#include <vector>

namespace Renderer
{

/**
 * @struct Vertex
 * @brief Interleaved vertex format used by Mesh.
 *
 * @details
 * Layout in memory:
 *  - position (vec3)
 *  - texCoord (vec2)
 *  - normal   (vec3)
 *
 * Total size: 32 bytes.
 */
struct Vertex
{
    glm::vec3 position; ///< World-space position.
    glm::vec2 texCoord; ///< Texture coordinates (UV).
    glm::vec3 normal;   ///< Vertex normal for lighting.
};

/**
 * @class Mesh
 * @brief Represents a single drawable geometry unit.
 *
 * @details
 * A Mesh:
 *  - Owns a GPU buffer (VAO/VBO/EBO via Buffer)
 *  - Uploads vertex + index data on construction
 *  - Knows how to issue its draw call
 *
 * Ownership:
 *  - Owns its Buffer
 *  - Non-copyable (GPU resource owner)
 *  - Movable
 *
 * Contract:
 *  - Shader must be bound before calling draw().
 */
class Mesh
{
  public:
    /**
     * @brief Constructs a mesh and uploads geometry to GPU.
     *
     * @param vertices Vertex array (interleaved format).
     * @param indices Index buffer for indexed drawing.
     */
    Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

    Mesh(const Mesh&)            = delete;
    Mesh& operator=(const Mesh&) = delete;

    Mesh(Mesh&&)            = default;
    Mesh& operator=(Mesh&&) = default;

    /**
     * @brief Issues draw call for this mesh.
     *
     * @param shader Shader program (must already be bound).
     */
    void draw(const Shader& shader) const;

    /**
     * @return Number of vertices.
     */
    size_t vertexCount() const
    {
        return m_vertexCount;
    }

    /**
     * @return Number of indices.
     */
    size_t indexCount() const
    {
        return m_indexCount;
    }

  private:
    /**
     * @brief Converts vertices to flat format and uploads to GPU.
     */
    void setupBuffer(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

    Buffer m_buffer;          ///< Owned GPU buffer.
    size_t m_vertexCount = 0; ///< CPU-side vertex count.
    size_t m_indexCount  = 0; ///< CPU-side index count.
};

} // namespace Renderer