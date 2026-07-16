# Web viewer is a Three.js rewrite, not a WebAssembly port of the C++ engine

Renderit is a native macOS C++/OpenGL renderer with no browser-facing surface. We want a polished, Sketchfab-quality web viewer (drag-to-orbit, day/night lighting, auto-fit on import, zip/folder import). Two paths existed: cross-compile the existing engine (+ Assimp) to WebAssembly/WebGL2 via Emscripten, or build a new web app in Three.js.

We chose Three.js. It ships mature multi-format loaders, PBR materials with image-based lighting, shadow maps, tone mapping, and orbit controls out of the box — the realistic path to Sketchfab-level quality in a reasonable timeframe. Porting the C++ engine would require cross-compiling Assimp to WASM (heavy, bloats download size, not a trivial Emscripten port) and hand-building IBL/tone-mapping/anti-aliasing from scratch — none of which exists in the engine today.

**Consequence:** the web viewer is a separate codebase from Renderit and does not reuse its shaders or rendering pipeline. Renderit continues as a standalone native learning project; domain knowledge (BRDF math, auto-fit logic, tangent-space normals) carries over conceptually but not as shared code.
