#pragma once

#include "renderer/Mesh.h"
#include <string>
#include <vector>
#include <memory>

namespace Scene {

class ObjLoader {
public:
    // Parse a .obj file and return all meshes found in it.
    // One OBJ file can contain multiple meshes separated by 'usemtl' directives.
    // Returns empty vector if the file cannot be opened.
    static std::vector<std::unique_ptr<Renderer::Mesh>> load(const std::string& path);

private:
    // Represents the raw data read from the OBJ file —
    // three separate indexed arrays, exactly as stored on disk.
    struct ObjData {
        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> texCoords;
        std::vector<glm::vec3> normals;
    };

    // One face vertex in OBJ notation: three separate indices
    // pointing into three separate arrays.
    struct FaceVertex {
        int posIdx = 0;
        int uvIdx  = 0;
        int normIdx = 0;

        // Needed to use FaceVertex as a key in std::unordered_map
        bool operator==(const FaceVertex& o) const {
            return posIdx == o.posIdx && uvIdx == o.uvIdx && normIdx == o.normIdx;
        }
    };

    // Hash function for FaceVertex so it can be used in unordered_map.
    // Combines the three indices into one hash value.
    struct FaceVertexHash {
        size_t operator()(const FaceVertex& fv) const {
            // Standard hash-combining technique (from boost::hash_combine)
            size_t seed = 0;
            auto combine = [&seed](int v) {
                seed ^= std::hash<int>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            };
            combine(fv.posIdx);
            combine(fv.uvIdx);
            combine(fv.normIdx);
            return seed;
        }
    };

    // Parse a single "f" line into a list of FaceVertex.
    // Handles triangles (3 entries) and quads (4 entries, auto-triangulated).
    // Handles all OBJ face formats:
    //   f 1 2 3              (position only)
    //   f 1/2 3/4 5/6        (position/texcoord)
    //   f 1/2/3 4/5/6 7/8/9  (position/texcoord/normal)
    //   f 1//2 3//4 5//6     (position//normal, no texcoord)
    static std::vector<FaceVertex> parseFaceLine(const std::string& line);

    // Parse one "v/vt/vn" token from a face line into a FaceVertex.
    static FaceVertex parseFaceVertex(const std::string& token);
};

} // namespace Scene