#include "ObjLoader.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>

namespace Scene {

// ─────────────────────────────────────────────────────────────────────────────
//  load() — main entry point
// ─────────────────────────────────────────────────────────────────────────────

std::vector<std::unique_ptr<Renderer::Mesh>>
ObjLoader::load(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[ObjLoader] Cannot open: " << path << "\n";
        return {};
    }

    std::cout << "[ObjLoader] Loading: " << path << "\n";

    // ── Step 1: Read all raw data from the file ───────────────────────────────
    // OBJ stores three completely separate arrays.
    // We read them all first before processing any faces.
    ObjData data;

    // ── Step 2: Build interleaved vertices ───────────────────────────────────
    // The key insight: each unique (posIdx, uvIdx, normIdx) combination
    // becomes one entry in our final vertex array.
    // This map tracks which combinations we've already seen.
    std::unordered_map<FaceVertex, uint32_t, FaceVertexHash> vertexCache;

    std::vector<Renderer::Vertex>  currentVertices;
    std::vector<uint32_t>          currentIndices;
    std::vector<std::unique_ptr<Renderer::Mesh>> meshes;

    std::string currentMaterial = "";
    std::string line;

    auto flushMesh = [&]() {
        // If we have geometry accumulated, build a Mesh from it
        if (!currentIndices.empty()) {
            std::cout << "[ObjLoader]   Mesh '" << currentMaterial << "': "
                      << currentVertices.size() << " vertices, "
                      << currentIndices.size() << " indices\n";
            meshes.push_back(
                std::make_unique<Renderer::Mesh>(currentVertices, currentIndices)
            );
            currentVertices.clear();
            currentIndices.clear();
            vertexCache.clear();
        }
    };

    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') continue;

        std::istringstream ss(line);
        std::string token;
        ss >> token;

        // ── "v" — vertex position ─────────────────────────────────────────
        if (token == "v") {
            glm::vec3 pos;
            ss >> pos.x >> pos.y >> pos.z;
            data.positions.push_back(pos);
        }

        // ── "vt" — texture coordinate (UV) ───────────────────────────────
        else if (token == "vt") {
            glm::vec2 uv;
            ss >> uv.x >> uv.y;
            // OBJ UVs have origin at bottom-left, OpenGL textures have origin
            // at top-left. Flip V to correct the orientation.
            uv.y = 1.0f - uv.y;
            data.texCoords.push_back(uv);
        }

        // ── "vn" — vertex normal ──────────────────────────────────────────
        else if (token == "vn") {
            glm::vec3 normal;
            ss >> normal.x >> normal.y >> normal.z;
            data.normals.push_back(normal);
        }

        // ── "usemtl" — material switch → flush current mesh, start new ───
        else if (token == "usemtl") {
            flushMesh();
            ss >> currentMaterial;
        }

        // ── "f" — face ────────────────────────────────────────────────────
        else if (token == "f") {
            // Get the rest of the line for face parsing
            std::string rest;
            std::getline(ss, rest);

            std::vector<FaceVertex> faceVerts = parseFaceLine(rest);

            // Fan triangulation for polygons with more than 3 vertices.
            // A quad (4 verts) becomes 2 triangles: (0,1,2) and (0,2,3)
            // A pentagon (5 verts) becomes 3 triangles: (0,1,2),(0,2,3),(0,3,4)
            // This is called "fan triangulation" — always anchored at vertex 0.
            for (size_t i = 1; i + 1 < faceVerts.size(); ++i) {
                // Each triangle in the fan: vertex 0, vertex i, vertex i+1
                for (const FaceVertex& fv : {faceVerts[0], faceVerts[i], faceVerts[i+1]}) {

                    // Check if this exact combination already exists
                    auto it = vertexCache.find(fv);
                    if (it != vertexCache.end()) {
                        // Already seen — reuse the existing index
                        currentIndices.push_back(it->second);
                    } else {
                        // New combination — create a new interleaved vertex

                        Renderer::Vertex vertex;

                        // OBJ indices are 1-based — subtract 1 for C++ arrays
                        // Negative indices in OBJ mean "relative to end" —
                        // e.g., -1 means last added. Handle both cases.
                        auto resolveIdx = [](int idx, int size) -> int {
                            return idx < 0 ? size + idx : idx - 1;
                        };

                        vertex.position = data.positions[
                            resolveIdx(fv.posIdx, data.positions.size())
                        ];

                        if (fv.uvIdx != 0 && !data.texCoords.empty()) {
                            vertex.texCoord = data.texCoords[
                                resolveIdx(fv.uvIdx, data.texCoords.size())
                            ];
                        }

                        if (fv.normIdx != 0 && !data.normals.empty()) {
                            vertex.normal = data.normals[
                                resolveIdx(fv.normIdx, data.normals.size())
                            ];
                        }

                        uint32_t newIndex = static_cast<uint32_t>(currentVertices.size());
                        currentVertices.push_back(vertex);
                        vertexCache[fv] = newIndex;
                        currentIndices.push_back(newIndex);
                    }
                }
            }
        }

        // Lines we intentionally skip:
        //   "mtllib" — material library filename (we'll use in Step 3)
        //   "o", "g" — object/group names (informational)
        //   "s"      — smooth shading flag (we use per-vertex normals instead)
    }

    // Flush the last mesh (no trailing "usemtl" to trigger it)
    flushMesh();

    std::cout << "[ObjLoader] Done. " << meshes.size() << " mesh(es) loaded.\n";
    return meshes;
}

// ─────────────────────────────────────────────────────────────────────────────
//  parseFaceLine — parse one "f" line into a list of FaceVertex
// ─────────────────────────────────────────────────────────────────────────────

std::vector<ObjLoader::FaceVertex>
ObjLoader::parseFaceLine(const std::string& line)
{
    std::vector<FaceVertex> result;
    std::istringstream ss(line);
    std::string token;

    while (ss >> token) {
        result.push_back(parseFaceVertex(token));
    }

    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
//  parseFaceVertex — parse one "v/vt/vn" token
// ─────────────────────────────────────────────────────────────────────────────

ObjLoader::FaceVertex
ObjLoader::parseFaceVertex(const std::string& token)
{
    // Possible formats:
    //   "1"        → position only
    //   "1/2"      → position/texcoord
    //   "1/2/3"    → position/texcoord/normal
    //   "1//3"     → position//normal (no texcoord — note the double slash)

    FaceVertex fv;
    std::stringstream ss(token);
    std::string part;

    // Read position index (always present)
    std::getline(ss, part, '/');
    if (!part.empty()) fv.posIdx = std::stoi(part);

    // Read texcoord index (may be empty in "1//3" case)
    if (std::getline(ss, part, '/')) {
        if (!part.empty()) fv.uvIdx = std::stoi(part);
    }

    // Read normal index (may not be present at all)
    if (std::getline(ss, part, '/')) {
        if (!part.empty()) fv.normIdx = std::stoi(part);
    }

    return fv;
}

} // namespace Scene