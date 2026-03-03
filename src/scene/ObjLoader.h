#pragma once

#include "renderer/Mesh.h"
#include "renderer/Texture.h"

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace Scene {

class ObjLoader {
public:
    static std::vector<std::unique_ptr<Renderer::Mesh>>
    load(const std::string& objPath);

private:
    struct ObjData {
        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> texCoords;
        std::vector<glm::vec3> normals;
    };

    struct FaceVertex {
        int posIdx  = 0;
        int uvIdx   = 0;
        int normIdx = 0;

        bool operator==(const FaceVertex& o) const {
            return posIdx == o.posIdx && uvIdx == o.uvIdx && normIdx == o.normIdx;
        }
    };

    struct FaceVertexHash {
        size_t operator()(const FaceVertex& fv) const {
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

    // Parse the .mtl file and return a map of materialName → texture path.
    // Uses a texture cache so the same image is only loaded once even
    // if referenced by multiple materials.
    static std::unordered_map<std::string, std::shared_ptr<Renderer::Texture>>
    loadMaterials(const std::string& mtlPath,
                  const std::string& baseDir);

    static std::vector<FaceVertex> parseFaceLine(const std::string& line);
    static FaceVertex              parseFaceVertex(const std::string& token);
};

} // namespace Scene