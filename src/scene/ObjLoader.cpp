#include "ObjLoader.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>

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
        if (line.empty() || line[0] == '#') continue;

        std::istringstream ss(line);
        std::string token;
        ss >> token;

        // "newmtl" — start of a new material definition
        if (token == "newmtl") {
            ss >> currentMaterial;
        }

        // "map_Kd" — diffuse texture map for the current material
        // This is the main color/albedo texture we want right now.
        // Other maps (map_Bump, map_Ks, etc.) we'll handle in later steps.
        else if (token == "map_Kd" && !currentMaterial.empty()) {
            std::string texFile;
            ss >> texFile;

            // Build full path: texture files sit next to the .mtl file
            std::string fullPath = baseDir + "/" + texFile;

            // Check cache first
            auto cacheIt = textureCache.find(fullPath);
            if (cacheIt != textureCache.end()) {
                materials[currentMaterial] = cacheIt->second;
                std::cout << "[ObjLoader]   Material '" << currentMaterial
                          << "' reusing cached texture: " << texFile << "\n";
            } else {
                try {
                    auto tex = std::make_shared<Renderer::Texture>(fullPath);
                    textureCache[fullPath]         = tex;
                    materials[currentMaterial]     = tex;
                } catch (const std::exception& e) {
                    std::cerr << "[ObjLoader]   Warning: " << e.what() << "\n";
                    // Continue — mesh will render without texture
                }
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
    std::ifstream file(objPath);
    if (!file.is_open()) {
        std::cerr << "[ObjLoader] Cannot open: " << objPath << "\n";
        return {};
    }

    std::cout << "[ObjLoader] Loading: " << objPath << "\n";

    // Extract the directory containing the .obj file.
    // All relative paths in the .mtl are resolved from here.
    std::string baseDir = ".";
    size_t lastSlash = objPath.find_last_of("/\\");
    if (lastSlash != std::string::npos)
        baseDir = objPath.substr(0, lastSlash);

    // Material name → Texture, populated when we hit "mtllib"
    std::unordered_map<std::string, std::shared_ptr<Renderer::Texture>> materials;

    ObjData data;

    std::unordered_map<FaceVertex, uint32_t, FaceVertexHash> vertexCache;
    std::vector<Renderer::Vertex>              currentVertices;
    std::vector<uint32_t>                      currentIndices;
    std::vector<std::unique_ptr<Renderer::Mesh>> meshes;

    std::string currentMaterial;
    std::string line;

    auto flushMesh = [&]() {
        if (currentIndices.empty()) return;

        // Look up the texture for this material (may be nullptr — that's fine)
        std::shared_ptr<Renderer::Texture> tex;
        auto matIt = materials.find(currentMaterial);
        if (matIt != materials.end())
            tex = matIt->second;

        std::cout << "[ObjLoader]   Mesh '" << currentMaterial << "': "
                  << currentVertices.size() << " vertices, "
                  << currentIndices.size()  << " indices"
                  << (tex ? " [textured]" : " [no texture]") << "\n";

        meshes.push_back(
            std::make_unique<Renderer::Mesh>(currentVertices, currentIndices, tex)
        );

        currentVertices.clear();
        currentIndices.clear();
        vertexCache.clear();
    };

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream ss(line);
        std::string token;
        ss >> token;

        if (token == "mtllib") {
            // Load the material library immediately so materials are
            // available when we start processing faces.
            std::string mtlFile;
            ss >> mtlFile;
            materials = loadMaterials(baseDir + "/" + mtlFile, baseDir);
        }

        else if (token == "v") {
            glm::vec3 pos;
            ss >> pos.x >> pos.y >> pos.z;
            data.positions.push_back(pos);
        }

        else if (token == "vt") {
            glm::vec2 uv;
            ss >> uv.x >> uv.y;
            uv.y = 1.0f - uv.y;  // flip V: OBJ origin is bottom-left, OpenGL top-left
            data.texCoords.push_back(uv);
        }

        else if (token == "vn") {
            glm::vec3 n;
            ss >> n.x >> n.y >> n.z;
            data.normals.push_back(n);
        }

        else if (token == "usemtl") {
            flushMesh();
            ss >> currentMaterial;
        }

        else if (token == "f") {
            std::string rest;
            std::getline(ss, rest);
            std::vector<FaceVertex> faceVerts = parseFaceLine(rest);

            // Fan triangulation for quads and n-gons
            for (size_t i = 1; i + 1 < faceVerts.size(); ++i) {
                for (const FaceVertex& fv : {faceVerts[0], faceVerts[i], faceVerts[i+1]}) {

                    auto it = vertexCache.find(fv);
                    if (it != vertexCache.end()) {
                        currentIndices.push_back(it->second);
                    } else {
                        Renderer::Vertex vertex;

                        auto resolveIdx = [](int idx, int size) -> int {
                            return idx < 0 ? size + idx : idx - 1;
                        };

                        vertex.position = data.positions[
                            resolveIdx(fv.posIdx, (int)data.positions.size())];

                        if (fv.uvIdx != 0 && !data.texCoords.empty())
                            vertex.texCoord = data.texCoords[
                                resolveIdx(fv.uvIdx, (int)data.texCoords.size())];

                        if (fv.normIdx != 0 && !data.normals.empty())
                            vertex.normal = data.normals[
                                resolveIdx(fv.normIdx, (int)data.normals.size())];

                        uint32_t newIndex = (uint32_t)currentVertices.size();
                        currentVertices.push_back(vertex);
                        vertexCache[fv] = newIndex;
                        currentIndices.push_back(newIndex);
                    }
                }
            }
        }
    }

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