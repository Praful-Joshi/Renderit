# Context Map

## Contexts

- [VS Code Extension](./vscode-extension/CONTEXT.md) — the flagship project: a VS Code extension embedding the Web Viewer's core in a webview
- [Web Viewer](./web/CONTEXT.md) — Three.js browser-based model viewer, shipped as its own GitHub Pages site
- [Educational Renderer](./educational/src/CONTEXT.md) — C++/OpenGL desktop renderer; the project's origin, now a self-contained learning reference (see `educational/README.md`)

## Relationships

- **VS Code Extension ↔ Web Viewer: not independent.** The extension imports `web/src/viewer/*.ts` directly and unmodified — same `Viewer`, `ImportResolver`, `ResourceResolver`, `LightingPresets`, `AutoFit`, `MetadataExtractor` code, just bundled into a webview instead of a browser tab. A fix to the shared viewer core fixes both without any extension-side change. See `docs/adr/0005-fast-follow-is-vscode-extension-not-browser-extension.md`.
- **Educational Renderer ↔ (Web Viewer / VS Code Extension): independent.** No shared code or runtime dependency. Related only by project intent (both render 3D models) and by domain knowledge that carries over conceptually (BRDF math, auto-fit logic, tangent-space normals) but not as shared code. See `docs/adr/0001-web-viewer-is-a-threejs-rewrite.md`.
