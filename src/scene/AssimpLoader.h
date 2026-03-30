#pragma once

#include "renderer/Model.h"
#include "renderer/Texture.h"

#include <assimp/scene.h>

#include <string>
#include <memory>
#include <unordered_map>

namespace Scene {

class AssimpLoader {
public:
    // Load a model file (OBJ, FBX, GLTF, DAE, ...) and return a Model.
    // Throws std::runtime_error if the file cannot be loaded.
    static std::unique_ptr<Renderer::Model> load(const std::string& path);

private:
    // Process one Assimp mesh node → our Mesh
    static std::unique_ptr<Renderer::Mesh> processMesh(
        const aiMesh*  mesh,
        const aiScene* scene,
        const std::string& modelDir,
        std::unordered_map<std::string, std::shared_ptr<Renderer::Texture>>& textureCache
    );

    // Build a Material from an Assimp material
    static std::shared_ptr<Renderer::Material> processMaterial(
        const aiMaterial* material,
        const std::string& modelDir,
        std::unordered_map<std::string, std::shared_ptr<Renderer::Texture>>& textureCache
    );

    // Load a texture of the given type from an Assimp material,
    // returning nullptr if that map doesn't exist.
    // Uses textureCache to avoid loading the same image twice.
    static std::shared_ptr<Renderer::Texture> loadTexture(
        const aiMaterial* material,
        aiTextureType     type,
        const std::string& modelDir,
        std::unordered_map<std::string, std::shared_ptr<Renderer::Texture>>& textureCache
    );

    // Fallback: scan the model directory for a file whose lowercase name
    // contains any of the given keywords (e.g. {"rough","roughness"}).
    // FBX exporters often embed texture paths in non-standard Assimp slots,
    // so typed lookups fail even when the file is sitting right next to the model.
    static std::shared_ptr<Renderer::Texture> findTextureByKeyword(
        const std::string& modelDir,
        const std::vector<std::string>& keywords,
        std::unordered_map<std::string, std::shared_ptr<Renderer::Texture>>& textureCache
    );
};

} // namespace Scene