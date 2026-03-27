#include "AssimpLoader.h"

#include "renderer/Material.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include <filesystem>
#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;

namespace Scene {

// ─────────────────────────────────────────────────────────────────────────────
//  load — entry point
// ─────────────────────────────────────────────────────────────────────────────

std::unique_ptr<Renderer::Model>
AssimpLoader::load(const std::string& path)
{
    Assimp::Importer importer;

    // Post-processing flags tell Assimp what to do while loading:
    const unsigned int flags =
        aiProcess_Triangulate           // convert quads/ngons to triangles
      | aiProcess_FlipUVs               // flip V: OBJ has (0,0) bottom-left,
                                        //         OpenGL expects top-left
      | aiProcess_GenSmoothNormals      // generate normals if missing
      | aiProcess_CalcTangentSpace      // compute tangent + bitangent per vertex
                                        // needed for normal mapping in Step 7
      | aiProcess_JoinIdenticalVertices // deduplicate shared vertices (index buffer)
      | aiProcess_ValidateDataStructure;// catch malformed files early

    const aiScene* scene = importer.ReadFile(path, flags);

    // Validate — Assimp returns nullptr or sets the incomplete flag on failure
    if (!scene || !scene->mRootNode ||
        (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE))
    {
        throw std::runtime_error(
            "[AssimpLoader] Failed to load: " + path +
            "\n  " + importer.GetErrorString()
        );
    }

    std::cout << "[AssimpLoader] Loading: " << path << "\n"
              << "  Meshes:    " << scene->mNumMeshes    << "\n"
              << "  Materials: " << scene->mNumMaterials << "\n"
              << "  Textures:  " << scene->mNumTextures  << "\n";

    // Resolve directory for relative texture paths
    std::string modelDir = ".";
    size_t lastSlash = path.find_last_of("/\\");
    if (lastSlash != std::string::npos)
        modelDir = path.substr(0, lastSlash);

    // Texture cache — shared across all meshes in this model.
    // If the cottage wall and roof both reference the same diffuse PNG,
    // it's loaded once and both Mesh objects share the same Texture via shared_ptr.
    std::unordered_map<std::string, std::shared_ptr<Renderer::Texture>> textureCache;

    // Assimp gives a scene graph (tree of nodes). We flatten it into a list
    // of meshes — for static models this is always correct. Skeletal animation
    // would need to preserve the hierarchy, but that's out of scope.
    std::vector<std::unique_ptr<Renderer::Mesh>> meshes;

    // Iterate all meshes in the scene (Assimp stores them in a flat array,
    // nodes reference them by index)
    for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        auto mesh = processMesh(scene->mMeshes[i], scene, modelDir, textureCache);
        if (mesh) meshes.push_back(std::move(mesh));
    }

    std::cout << "[AssimpLoader] Done. " << meshes.size() << " mesh(es).\n";
    return std::make_unique<Renderer::Model>(std::move(meshes));
}

// ─────────────────────────────────────────────────────────────────────────────
//  processMesh — convert one aiMesh → our Mesh
// ─────────────────────────────────────────────────────────────────────────────

std::unique_ptr<Renderer::Mesh>
AssimpLoader::processMesh(
    const aiMesh*  aiMesh,
    const aiScene* scene,
    const std::string& modelDir,
    std::unordered_map<std::string, std::shared_ptr<Renderer::Texture>>& textureCache)
{
    std::vector<Renderer::Vertex>  vertices;
    std::vector<uint32_t>          indices;

    vertices.reserve(aiMesh->mNumVertices);

    // ── Vertices ──────────────────────────────────────────────────────────────
    for (unsigned int i = 0; i < aiMesh->mNumVertices; ++i) {
        Renderer::Vertex v;

        // Position — always present
        v.position = {
            aiMesh->mVertices[i].x,
            aiMesh->mVertices[i].y,
            aiMesh->mVertices[i].z
        };

        // Normals — present if aiProcess_GenSmoothNormals was set
        if (aiMesh->HasNormals()) {
            v.normal = {
                aiMesh->mNormals[i].x,
                aiMesh->mNormals[i].y,
                aiMesh->mNormals[i].z
            };
        }

        // UV coordinates — Assimp supports 8 UV channels.
        // We always use channel 0 (the primary UV map).
        if (aiMesh->mTextureCoords[0]) {
            v.texCoord = {
                aiMesh->mTextureCoords[0][i].x,
                aiMesh->mTextureCoords[0][i].y
            };
        }

        // Tangent + Bitangent — computed by aiProcess_CalcTangentSpace.
        // These form the TBN matrix used in Step 7 for normal mapping.
        if (aiMesh->HasTangentsAndBitangents()) {
            v.tangent = {
                aiMesh->mTangents[i].x,
                aiMesh->mTangents[i].y,
                aiMesh->mTangents[i].z
            };
            v.bitangent = {
                aiMesh->mBitangents[i].x,
                aiMesh->mBitangents[i].y,
                aiMesh->mBitangents[i].z
            };
        }

        vertices.push_back(v);
    }

    // ── Indices ───────────────────────────────────────────────────────────────
    // Assimp gives faces (triangles after aiProcess_Triangulate).
    // Each face has exactly 3 indices.
    indices.reserve(aiMesh->mNumFaces * 3);
    for (unsigned int i = 0; i < aiMesh->mNumFaces; ++i) {
        const aiFace& face = aiMesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j)
            indices.push_back(face.mIndices[j]);
    }

    // ── Material ──────────────────────────────────────────────────────────────
    std::shared_ptr<Renderer::Material> material;
    if (aiMesh->mMaterialIndex >= 0) {
        const aiMaterial* aiMat = scene->mMaterials[aiMesh->mMaterialIndex];
        material = processMaterial(aiMat, modelDir, textureCache);
    }

    std::cout << "[AssimpLoader]   Mesh '" << aiMesh->mName.C_Str() << "': "
              << vertices.size() << " verts, "
              << indices.size()  << " indices"
              << (material && material->diffuseMap ? " [diffuse]" : "")
              << (material && material->normalMap  ? " [normal]"  : "")
              << "\n";

    return std::make_unique<Renderer::Mesh>(vertices, indices, material);
}

// ─────────────────────────────────────────────────────────────────────────────
//  processMaterial — convert aiMaterial → our Material
// ─────────────────────────────────────────────────────────────────────────────

std::shared_ptr<Renderer::Material>
AssimpLoader::processMaterial(
    const aiMaterial* aiMat,
    const std::string& modelDir,
    std::unordered_map<std::string, std::shared_ptr<Renderer::Texture>>& textureCache)
{
    auto material = std::make_shared<Renderer::Material>();

    // Material name (for debugging)
    aiString name;
    aiMat->Get(AI_MATKEY_NAME, name);
    material->name = name.C_Str();

    // ── Diffuse map ───────────────────────────────────────────────────────────
    material->diffuseMap = loadTexture(aiMat, aiTextureType_DIFFUSE,
                                       modelDir, textureCache);

    // ── Normal map ────────────────────────────────────────────────────────────
    // Assimp uses aiTextureType_NORMALS for normal maps in most formats.
    // Some exporters use aiTextureType_HEIGHT instead — we try both.
    material->normalMap = loadTexture(aiMat, aiTextureType_NORMALS,
                                      modelDir, textureCache);
    if (!material->normalMap)
        material->normalMap = loadTexture(aiMat, aiTextureType_HEIGHT,
                                          modelDir, textureCache);

    // ── Roughness map ─────────────────────────────────────────────────────────
    // glTF/DAE exporters use DIFFUSE_ROUGHNESS. Some OBJ/FBX exporters pack
    // roughness into the SHININESS slot instead.
    material->roughnessMap = loadTexture(aiMat, aiTextureType_DIFFUSE_ROUGHNESS,
                                         modelDir, textureCache);
    if (!material->roughnessMap)
        material->roughnessMap = loadTexture(aiMat, aiTextureType_SHININESS,
                                             modelDir, textureCache);

    // ── Metallic map ──────────────────────────────────────────────────────────
    material->metallicMap = loadTexture(aiMat, aiTextureType_METALNESS,
                                        modelDir, textureCache);

    // ── AO map ────────────────────────────────────────────────────────────────
    // Ambient occlusion is sometimes packed into the LIGHTMAP slot.
    material->aoMap = loadTexture(aiMat, aiTextureType_AMBIENT_OCCLUSION,
                                  modelDir, textureCache);
    if (!material->aoMap)
        material->aoMap = loadTexture(aiMat, aiTextureType_LIGHTMAP,
                                      modelDir, textureCache);

    // ── Scalar properties ─────────────────────────────────────────────────────
    aiColor3D color;
    if (aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS)
        material->diffuseColor = { color.r, color.g, color.b };

    if (aiMat->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS)
        material->specularColor = { color.r, color.g, color.b };

    float shininess = 32.0f;
    if (aiMat->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS)
        material->shininess = shininess;

    return material;
}

// ─────────────────────────────────────────────────────────────────────────────
//  loadTexture — load one texture map from an aiMaterial
// ─────────────────────────────────────────────────────────────────────────────

std::shared_ptr<Renderer::Texture>
AssimpLoader::loadTexture(
    const aiMaterial* material,
    aiTextureType     type,
    const std::string& modelDir,
    std::unordered_map<std::string, std::shared_ptr<Renderer::Texture>>& textureCache)
{
    // Check if this material has a texture of the requested type
    if (material->GetTextureCount(type) == 0)
        return nullptr;

    aiString texPath;
    material->GetTexture(type, 0, &texPath); // index 0 = first texture of this type

    // ── Sanitize path ─────────────────────────────────────────────────────────
    // MTL files exported from Windows often embed a subdirectory prefix using
    // backslashes (e.g. "tEXTURE\BOdy Skin Base Color.png"). On macOS/Linux
    // the backslash is not a directory separator so the full string is treated
    // as a literal filename — which of course doesn't exist.
    //
    // Fix: convert all backslashes to forward slashes, then strip everything
    // up to (and including) the last slash, keeping only the bare filename.
    // Then resolve that filename relative to the directory the model lives in.
    std::string rawPath = texPath.C_Str();
    std::replace(rawPath.begin(), rawPath.end(), '\\', '/');
    std::string filename = fs::path(rawPath).filename().string();
    std::string fullPath = modelDir + "/" + filename;

    // Cache lookup — return existing texture if already loaded
    auto it = textureCache.find(fullPath);
    if (it != textureCache.end())
        return it->second;

    // ── Exact load ────────────────────────────────────────────────────────────
    if (fs::exists(fullPath)) {
        try {
            auto tex = std::make_shared<Renderer::Texture>(fullPath);
            textureCache[fullPath] = tex;
            return tex;
        } catch (const std::exception& e) {
            std::cerr << "[AssimpLoader] Warning: " << e.what() << "\n";
            return nullptr;
        }
    }

    // ── Case-insensitive fallback ─────────────────────────────────────────────
    // Some exporters mangle capitalisation (e.g. "FACE Base Color.png" in the
    // MTL vs "FACE Base Color apha.png" on disk). Scan the model directory for
    // any file whose name matches case-insensitively.
    std::string lowerTarget = filename;
    std::transform(lowerTarget.begin(), lowerTarget.end(),
                   lowerTarget.begin(), ::tolower);

    std::string bestMatch;
    try {
        for (const auto& entry : fs::directory_iterator(modelDir)) {
            if (!entry.is_regular_file()) continue;
            std::string candidate = entry.path().filename().string();
            std::string lowerCandidate = candidate;
            std::transform(lowerCandidate.begin(), lowerCandidate.end(),
                           lowerCandidate.begin(), ::tolower);
            if (lowerCandidate == lowerTarget) {
                bestMatch = entry.path().string();
                break;
            }
        }
    } catch (...) {}

    if (!bestMatch.empty()) {
        std::cout << "[AssimpLoader] Note: resolved '" << filename
                  << "' → '" << fs::path(bestMatch).filename().string()
                  << "' (case-insensitive match)\n";
        try {
            auto tex = std::make_shared<Renderer::Texture>(bestMatch);
            textureCache[fullPath] = tex;  // cache under the original key
            return tex;
        } catch (const std::exception& e) {
            std::cerr << "[AssimpLoader] Warning: " << e.what() << "\n";
            return nullptr;
        }
    }

    std::cerr << "[AssimpLoader] Warning: texture not found: " << fullPath << "\n";
    return nullptr;
}

} // namespace Scene