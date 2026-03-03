#include "ObjLoader.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>

// Small helper utilities kept local to this translation unit.
namespace {

bool isEmptyOrComment(const std::string& line) {
    return line.empty() || line[0] == '#';
}

int resolveIndex(int idx, int size) {
    return idx < 0 ? size + idx : idx - 1;
}

std::shared_ptr<Renderer::Texture> loadTextureCached(
    const std::string& fullPath,
    std::unordered_map<std::string, std::shared_ptr<Renderer::Texture>>& cache)
{
    auto it = cache.find(fullPath);
    if (it != cache.end()) return it->second;

    try {
        auto tex = std::make_shared<Renderer::Texture>(fullPath);
        cache[fullPath] = tex;
        return tex;
    } catch (const std::exception& e) {
        std::cerr << "[ObjLoader]   Warning: " << e.what() << "\n";
        return nullptr;
    }
}

} // anonymous namespace

namespace Scene {

// ─────────────────────────────────────────────────────────────────────────────
//  loadMaterials — parse .mtl file, load textures
// ─────────────────────────────────────────────────────────────────────────────

std::unordered_map<std::string, std::shared_ptr<Renderer::Texture>>
ObjLoader::loadMaterials(const std::string& mtlPath, const std::string& baseDir)
{
    std::unordered_map<std::string, std::shared_ptr<Renderer::Texture>> materials;

    std::ifstream file(mtlPath);
    if (!file.is_open()) {
        std::cerr << "[ObjLoader] Warning: cannot open MTL: " << mtlPath << "\n";
        return materials;
    }

    std::cout << "[ObjLoader] Parsing MTL: " << mtlPath << "\n";

    // Texture cache — if two materials reference the same image file,
    // we load it once and share the same Texture object via shared_ptr.
    std::unordered_map<std::string, std::shared_ptr<Renderer::Texture>> textureCache;

    std::string currentMaterial;
    std::string line;

    while (std::getline(file, line)) {
        if (isEmptyOrComment(line)) continue;

        std::istringstream ss(line);
        std::string token;
        ss >> token;

        if (token == "newmtl") {
            ss >> currentMaterial;
            continue;
        }

        // "map_Kd" — diffuse / albedo texture for the current material
        if (token == "map_Kd" && !currentMaterial.empty()) {
            std::string texFile;
            ss >> texFile;
            std::string fullPath = baseDir + "/" + texFile;
            auto tex = loadTextureCached(fullPath, textureCache);
            if (tex) {
                materials[currentMaterial] = tex;
                std::cout << "[ObjLoader]   Material '" << currentMaterial
                          << "' loaded texture: " << texFile << "\n";
            }
        }
    }

    return materials;
}

// ─────────────────────────────────────────────────────────────────────────────
//  load — main entry point
// ─────────────────────────────────────────────────────────────────────────────

std::vector<std::unique_ptr<Renderer::Mesh>>
ObjLoader::load(const std::string& objPath)
{
    // Find and parse the .obj file
    std::ifstream file(objPath);
    if (!file.is_open()) {
        std::cerr << "[ObjLoader] Cannot open: " << objPath << "\n";
        return {};
    }

    std::cout << "[ObjLoader] Loading: " << objPath << "\n";

    // Extract directory for resolving material relative paths
    std::string baseDir = ".";
    size_t lastSlash = objPath.find_last_of("/\\");
    if (lastSlash != std::string::npos) baseDir = objPath.substr(0, lastSlash);
    // Find and parse the .obj file

    // Declare data structures for parsing and constructing meshes
    std::unordered_map<std::string, std::shared_ptr<Renderer::Texture>> materials;
    ObjData data;
    std::unordered_map<FaceVertex, uint32_t, FaceVertexHash> vertexCache;
    std::vector<Renderer::Vertex> currentVertices;
    std::vector<uint32_t> currentIndices;
    std::vector<std::unique_ptr<Renderer::Mesh>> meshes;
    std::string currentMaterial;
    std::string line;
    // Declare data structures for parsing and constructing meshes

    // Helper to flush current mesh data into a Mesh object and reset for the next one.
    auto flushMesh = [&]() {
        if (currentIndices.empty()) return;
        std::shared_ptr<Renderer::Texture> tex = nullptr;
        auto it = materials.find(currentMaterial);
        if (it != materials.end()) tex = it->second;

        std::cout << "[ObjLoader]   Mesh '" << currentMaterial << "': "
                  << currentVertices.size() << " vertices, " << currentIndices.size()
                  << (tex ? " [textured]" : " [no texture]") << "\n";

        meshes.push_back(std::make_unique<Renderer::Mesh>(currentVertices, currentIndices, tex));
        currentVertices.clear();
        currentIndices.clear();
        vertexCache.clear();
    };
    // Helper to flush current mesh data into a Mesh object and reset for the next one.

    // Main parsing loop
    while (std::getline(file, line)) {
        if (isEmptyOrComment(line)) continue;

        std::istringstream ss(line);
        std::string token;
        ss >> token;

        if (token == "mtllib") {
            std::string mtlFile;
            ss >> mtlFile;
            materials = loadMaterials(baseDir + "/" + mtlFile, baseDir);
            continue;
        }

        if (token == "v") {
            glm::vec3 pos; ss >> pos.x >> pos.y >> pos.z;
            data.positions.push_back(pos);
            continue;
        }

        if (token == "vt") {
            glm::vec2 uv; ss >> uv.x >> uv.y; uv.y = 1.0f - uv.y;
            data.texCoords.push_back(uv);
            continue;
        }

        if (token == "vn") {
            glm::vec3 n; ss >> n.x >> n.y >> n.z;
            data.normals.push_back(n);
            continue;
        }

        if (token == "usemtl") {
            flushMesh(); ss >> currentMaterial; continue;
        }

        if (token == "f") {
            std::string rest; std::getline(ss, rest);
            std::vector<FaceVertex> faceVerts = parseFaceLine(rest);

            // Triangulate polygon (fan) and emit vertices/indices
            for (size_t i = 1; i + 1 < faceVerts.size(); ++i) {
                const FaceVertex corner[3] = { faceVerts[0], faceVerts[i], faceVerts[i+1] };
                for (int k = 0; k < 3; ++k) {
                    const FaceVertex& fv = corner[k];

                    auto it = vertexCache.find(fv);
                    if (it != vertexCache.end()) {
                        currentIndices.push_back(it->second);
                        continue;
                    }

                    Renderer::Vertex vertex{};
                    if (fv.posIdx != 0 && !data.positions.empty())
                        vertex.position = data.positions[resolveIndex(fv.posIdx, (int)data.positions.size())];

                    if (fv.uvIdx != 0 && !data.texCoords.empty())
                        vertex.texCoord = data.texCoords[resolveIndex(fv.uvIdx, (int)data.texCoords.size())];

                    if (fv.normIdx != 0 && !data.normals.empty())
                        vertex.normal   = data.normals[resolveIndex(fv.normIdx, (int)data.normals.size())];

                    uint32_t newIndex = (uint32_t)currentVertices.size();
                    currentVertices.push_back(vertex);
                    vertexCache[fv] = newIndex;
                    currentIndices.push_back(newIndex);
                }
            }
        }
    }
    // Main parsing loop

    // Flush any remaining mesh data after the loop
    flushMesh();

    std::cout << "[ObjLoader] Done. " << meshes.size() << " mesh(es) loaded.\n";
    return meshes;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Face parsing helpers — unchanged from before
// ─────────────────────────────────────────────────────────────────────────────

std::vector<ObjLoader::FaceVertex>
ObjLoader::parseFaceLine(const std::string& line)
{
    std::vector<FaceVertex> result;
    std::istringstream ss(line);
    std::string token;
    while (ss >> token)
        result.push_back(parseFaceVertex(token));
    return result;
}

ObjLoader::FaceVertex
ObjLoader::parseFaceVertex(const std::string& token)
{
    FaceVertex fv;
    std::stringstream ss(token);
    std::string part;

    std::getline(ss, part, '/');
    if (!part.empty()) fv.posIdx = std::stoi(part);

    if (std::getline(ss, part, '/'))
        if (!part.empty()) fv.uvIdx = std::stoi(part);

    if (std::getline(ss, part, '/'))
        if (!part.empty()) fv.normIdx = std::stoi(part);

    return fv;
}

} // namespace Scene