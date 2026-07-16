# Web viewer supports Three.js's native loader formats only, not full Assimp parity

The native Renderit app uses Assimp, which supports 40+ formats. The web viewer, per ADR-0001, doesn't reuse Assimp — a WASM build (`assimpjs`) was considered to keep format parity, but rejected: it reintroduces the exact download-size/complexity cost ADR-0001 avoided, and Assimp's generic material model still needs hand-mapping onto Three.js's PBR materials either way, so it doesn't even save implementation work.

**Decision:** support only Three.js's native loaders — glTF/GLB, OBJ+MTL, FBX, STL, Collada. This covers the large majority of real-world consumer models (including everything Sketchfab itself commonly exports/accepts) with no extra WASM weight and direct, well-tested mapping to Three.js's PBR material system.

**Consequence:** formats outside this list (e.g. `.blend`, `.3ds`, `.dae` variants Three.js doesn't parse, CAD formats) will not open in the web viewer even though the native Assimp-based app can load them. Revisit only if a specific missing format turns out to matter in practice.
