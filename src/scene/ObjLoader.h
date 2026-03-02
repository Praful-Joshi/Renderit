#pragma once

#include "renderer/Mesh.h"
#include <memory>
#include <string>
#include <vector>

namespace Scene
{

/**
 * @class ObjLoader
 * @brief Static utility class for loading Wavefront OBJ files.
 *
 * @details
 * Responsible for:
 *  - Parsing vertex positions, texture coordinates, and normals
 *  - Parsing face definitions in all supported OBJ formats
 *  - Converting indexed OBJ data into interleaved GPU-ready vertices
 *  - Triangulating polygonal faces using fan triangulation
 *  - Splitting meshes based on `usemtl` directives
 *
 * Design:
 *  - Pure static loader (no instances required)
 *  - Returns fully constructed Renderer::Mesh objects
 *  - Uses vertex deduplication via hashing
 *
 * Supported face formats:
 *   - f v1 v2 v3
 *   - f v1/vt1 v2/vt2 v3/vt3
 *   - f v1/vt1/vn1 ...
 *   - f v1//vn1 ...
 *
 * Notes:
 *  - OBJ indices are 1-based
 *  - Negative indices (relative indexing) are supported
 *  - UV Y-coordinate is flipped to match OpenGL texture space
 */
class ObjLoader
{
  public:
    /**
     * @brief Loads a Wavefront OBJ file from disk.
     *
     * @param path Path to the .obj file.
     *
     * @return A vector of unique_ptr<Mesh>.
     *         Multiple meshes are produced if the OBJ file
     *         contains multiple `usemtl` sections.
     *
     * @note Returns empty vector if file cannot be opened.
     */
    static std::vector<std::unique_ptr<Renderer::Mesh>> load(const std::string& path);

  private:
    /**
     * @struct ObjData
     * @brief Raw vertex arrays extracted directly from the OBJ file.
     *
     * @details
     * OBJ files store three independent indexed arrays:
     *  - Positions
     *  - Texture coordinates
     *  - Normals
     *
     * Faces reference these arrays separately.
     */
    struct ObjData
    {
        std::vector<glm::vec3> positions; ///< Vertex positions (v)
        std::vector<glm::vec2> texCoords; ///< Texture coordinates (vt)
        std::vector<glm::vec3> normals;   ///< Vertex normals (vn)
    };

    /**
     * @struct FaceVertex
     * @brief Represents one vertex entry inside a face definition.
     *
     * @details
     * Each face vertex references up to three independent indices:
     *  - Position index
     *  - Texture coordinate index
     *  - Normal index
     *
     * These indices are later combined into a single interleaved vertex.
     */
    struct FaceVertex
    {
        int posIdx  = 0; ///< Index into positions array
        int uvIdx   = 0; ///< Index into texCoords array
        int normIdx = 0; ///< Index into normals array

        /**
         * @brief Equality operator required for unordered_map key usage.
         */
        bool operator==(const FaceVertex& o) const
        {
            return posIdx == o.posIdx && uvIdx == o.uvIdx && normIdx == o.normIdx;
        }
    };

    /**
     * @struct FaceVertexHash
     * @brief Hash function for FaceVertex.
     *
     * @details
     * Combines the three indices into a single hash value
     * using a standard hash-combine technique.
     *
     * Required for use in std::unordered_map.
     */
    struct FaceVertexHash
    {
        size_t operator()(const FaceVertex& fv) const
        {
            size_t seed = 0;

            auto combine = [&seed](int v)
            { seed ^= std::hash<int>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2); };

            combine(fv.posIdx);
            combine(fv.uvIdx);
            combine(fv.normIdx);

            return seed;
        }
    };

    /**
     * @brief Parses a full face line (after the "f" token).
     *
     * @param line The remainder of the face line.
     *
     * @return Vector of FaceVertex entries.
     *
     * @details
     * Supports:
     *  - Triangles
     *  - Quads
     *  - Polygons (fan triangulated later)
     *
     * Does not perform triangulation — only parsing.
     */
    static std::vector<FaceVertex> parseFaceLine(const std::string& line);

    /**
     * @brief Parses a single face vertex token.
     *
     * @param token One token of format:
     *        - "v"
     *        - "v/vt"
     *        - "v/vt/vn"
     *        - "v//vn"
     *
     * @return Parsed FaceVertex structure.
     */
    static FaceVertex parseFaceVertex(const std::string& token);
};

} // namespace Scene