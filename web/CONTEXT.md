# Web Viewer

A browser-based 3D model viewer built in Three.js/TypeScript, distinct from the native `src/` renderer (see `docs/adr/0001-web-viewer-is-a-threejs-rewrite.md`). Ships first as a GitHub Pages website, later reused as the core of a VS Code extension (see `docs/adr/0005-fast-follow-is-vscode-extension-not-browser-extension.md`).

## Language

**Viewer**:
The reusable core component (scene, camera, renderer, lighting, controls) that both the website and the VS Code extension's webview embed. Not tied to either shell.
_Avoid_: App, renderer (ambiguous with the native `src/` renderer), engine.

**Import**:
The act of bringing a model into the Viewer — via drag-and-drop or a file picker, as a single model file, a zip, or a folder (flat or with subfolders).
_Avoid_: Upload (nothing leaves the browser — everything is processed client-side), load.

**Auto-fit**:
Bounding-box-based centering and camera framing applied to every imported model, regardless of format, so it fills the majority of the viewport. Always solvable, unlike orientation.
_Avoid_: Auto-scale (fit covers both centering and scaling together).

**Auto-orientation**:
Format-metadata-driven up-axis correction applied on import (glTF/FBX/Collada can self-report; OBJ/STL default to Y-up with no correction attempted). See `docs/adr/0003-auto-orientation-scope.md`. Not a heuristic/guessing system.
_Avoid_: Auto-rotate (that term is reserved for the idle spin behavior, a different concept).

**Idle auto-rotate**:
The slow automatic spin that plays before the user's first drag interaction, then stops permanently for that session once the user takes control of the camera.

**Lighting preset**:
One of two HDRI-based image-based-lighting configurations — Day or Night — swapped via the settings panel. Drives lighting/reflections (`scene.environment`) regardless of the current Background mode.
_Avoid_: Environment, skybox — "environment map" is fine as a Three.js implementation detail, but the user-facing concept is the Lighting preset.

**Background mode**:
Whether the visible `scene.background` is the fixed neutral studio backdrop (Studio, the default) or the active Lighting preset's own HDRI image (HDRI), toggled independently of the Lighting preset itself in the settings panel. Lighting preset picks *which* HDRI lights the scene; Background mode picks whether that HDRI is also *visible* — the two are separate knobs.
_Avoid_: Skybox, Environment toggle (this project's term is "Background mode").

**Showcase model**:
The default model bundled with the site and pre-loaded into the Viewer before a visitor imports anything, so the page demonstrates quality immediately.
_Avoid_: Demo model, sample model.

**Settings panel**:
The sidebar UI outside the viewport: scale slider, rotation sliders (X/Y/Z), lighting preset toggle, background mode toggle, reset-view button.

**Metadata panel**:
Displays file name & format, file size, triangle/vertex count, and structural details (mesh/material/texture counts, bounding-box dimensions) for the currently imported model.
