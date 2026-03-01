#pragma once

#include "Buffer.h"
#include "Shader.h"

#include <glm/glm.hpp>
#include <vector>
#include <string>

namespace Renderer {

// One vertex: the interleaved data layout we upload to the GPU.
// Having a struct makes it clear what a vertex IS rather than
// treating it as a raw bag of floats.
struct Vertex {
    glm::vec3 position;
    glm::vec2 texCoord;   // UV — needed for textures (Step 3)
    glm::vec3 normal;     // needed for lighting (Step 5)
};

// A Mesh is one drawable unit of geometry.
// It owns its GPU buffers and knows how to draw itself given a shader.
// This is the atom everything else (Model, Scene) is built from.
class Mesh {
public:
    Mesh(const std::vector<Vertex>& vertices,
         const std::vector<uint32_t>& indices);

    // Non-copyable — owns GPU resources
    Mesh(const Mesh&)            = delete;
    Mesh& operator=(const Mesh&) = delete;

    // Movable
    Mesh(Mesh&&)            = default;
    Mesh& operator=(Mesh&&) = default;

    // Upload geometry to GPU and issue the draw call.
    // Shader must already be bound before calling draw().
    void draw(const Shader& shader) const;

    // Accessors for debugging / bounding box computation later
    size_t vertexCount() const { return m_vertexCount; }
    size_t indexCount()  const { return m_indexCount; }

private:
    void setupBuffer(const std::vector<Vertex>& vertices,
                     const std::vector<uint32_t>& indices);

    Buffer m_buffer;
    size_t m_vertexCount = 0;
    size_t m_indexCount  = 0;
};

} // namespace Renderer