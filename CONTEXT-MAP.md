# Context Map

## Contexts

- [Native Renderer](./src/CONTEXT.md) — C++/OpenGL desktop renderer (Renderit)
- [Web Viewer](./web/CONTEXT.md) — Three.js browser-based model viewer

## Relationships

- **Independent.** No shared code or runtime dependency between the two contexts. Related only by project intent (both render 3D models) and by domain knowledge that carries over conceptually (BRDF math, auto-fit logic, tangent-space normals) but not as shared code. See `docs/adr/0001-web-viewer-is-a-threejs-rewrite.md`.
