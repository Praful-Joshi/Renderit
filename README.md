<div align="center">

# Renderit — 3D Model Viewer for VS Code

View glTF/GLB, OBJ, FBX, STL, and Collada models directly inside VS Code — single-click open, zip/folder import, real-time lighting, no separate app.

[![CI](https://github.com/Praful-Joshi/Renderit/actions/workflows/test-vscode-extension.yml/badge.svg)](https://github.com/Praful-Joshi/Renderit/actions/workflows/test-vscode-extension.yml)
[![License: MIT](https://img.shields.io/github/license/Praful-Joshi/Renderit)](LICENSE)

**[VS Code Marketplace](#)** · **[Try the web viewer](https://praful-joshi.github.io/Renderit/)** · **[Quickstart](#quickstart)**

<img src="docs/images/zip_support.gif" alt="Renderit's viewer, shown here on the web — same rendering core the VS Code extension embeds" width="800">

</div>

---

## Features

- **Single/double-click open** — `.gltf`, `.glb`, `.obj`, `.fbx`, `.stl`, and `.dae` files open directly in a Renderit tab, no import dialog, no extra step.
- **Right-click "Open with Renderit"** on a `.zip` file or a folder in the Explorer, with automatic texture resolution whether files are flat or organized into subfolders.
- **Orbit, zoom, and reset** — click-drag to orbit with damping, scroll/pinch to zoom, idle auto-rotate before you interact, one-click reset to the framed view.
- **Day/Night lighting** and **Studio/HDRI background** — real image-based lighting, not flat directional lights.
- **Scale and rotation controls** for correcting formats (OBJ, STL) with no reliable up-axis metadata.
- **Metadata panel** — file size, format, triangle/vertex counts, mesh/material/texture counts, bounding-box dimensions.
- **State persists** across tab switches — camera position, scale, rotation, lighting, and background survive switching to another tab and back.

## Supported formats

| Format | Extension | Notes |
|---|---|---|
| glTF / GLB | `.gltf`, `.glb` | Full support, including embedded and external buffers/textures |
| Wavefront OBJ | `.obj` | Companion `.mtl` resolved automatically when present |
| Autodesk FBX | `.fbx` | |
| STL | `.stl` | Geometry only — STL has no material/color data |
| Collada | `.dae` | |
| Zip archive | `.zip` | Any of the above, plus textures, flat or in subfolders |
| Folder | — | Same resolution as zip, for an already-unpacked export |

## Usage

**Open a single model file** — double-click it in the Explorer (or single-click, VS Code's own file-association behavior applies). Renderit opens as the default editor for all six supported extensions.

**Open a zip or folder** — right-click it in the Explorer and choose **Open with Renderit**. This is the way to go for an export that includes textures alongside the model, whether it's a `.zip` straight from wherever you downloaded it or an already-extracted folder.

Once a model is open, the sidebar has:
- **Scale** and **Rotation X/Y/Z** sliders, for models that need a manual nudge.
- **Day** / **Night** lighting toggle, and **Studio** / **HDRI** background toggle.
- **Reset view**, to snap the camera back to the initial framing.
- A **Metadata** panel with the file's structural details.

## Requirements

VS Code 1.57.0 or later. No other setup — the extension bundles everything it needs.

## Known limitations

- No progress indicator while a large model is being decompressed/parsed — for a big zip or folder, the viewport can appear idle for a few seconds before the model appears.
- Materials with no embedded texture can pick up unexpected reflections from the active lighting preset's HDRI environment — this is a real rendering behavior (untextured materials reflect the current environment map), not a crash or a bug in file parsing, but it can look surprising on models that rely on flat/placeholder materials.
- Doesn't yet survive a full VS Code window restart — state persistence (above) covers switching tabs within the same session, not reopening VS Code from scratch.

See [`vscode-extension/CHANGELOG.md`](vscode-extension/CHANGELOG.md) for release notes.

## Quickstart

### Install (once published)

Search "Renderit 3D Viewer" in the Extensions view, or install from the Marketplace link above.

### Run from source

```bash
git clone https://github.com/Praful-Joshi/Renderit.git
cd Renderit/vscode-extension
npm install
npm run build
```

Then open `vscode-extension/` in VS Code and press **F5** — this launches an Extension Development Host with `test/fixtures/` as its workspace, so you can immediately double-click a sample model or right-click a sample zip/folder to try it.

## Development

This repo has three independent sub-projects. Each has its own `package.json`/build system — `cd` into it and follow its own scripts.

<table>
<tr><th></th><th>What it is</th><th>Build &amp; test</th></tr>
<tr>
<td><code>vscode-extension/</code></td>
<td>The VS Code extension (this README's subject) — TypeScript + Vite, embeds the web viewer's core in a webview.</td>
<td>

```bash
cd vscode-extension
npm install
npm run typecheck
npm test                  # unit tests (Vitest)
npm run test:integration  # real headless VS Code (@vscode/test-electron)
npm run build
```

</td>
</tr>
<tr>
<td><code>web/</code></td>
<td>The browser-based viewer, shipped as its own static site at <a href="https://praful-joshi.github.io/Renderit/">praful-joshi.github.io/Renderit</a>. The extension imports this project's <code>src/viewer/*.ts</code> directly and unmodified.</td>
<td>

```bash
cd web
npm install
npm run typecheck
npm test
npm run dev    # local dev server
npm run build
```

</td>
</tr>
<tr>
<td><code>educational/</code></td>
<td>Where this project started: a from-scratch C++/OpenGL renderer, kept as a self-contained learning reference. See <a href="educational/README.md">its own README</a> for the full roadmap and architecture notes.</td>
<td>

```bash
cd educational
mkdir build && cd build
cmake ..
make
./OpenGLRenderer
```

</td>
</tr>
</table>

Domain glossary and architecture decisions for each context live under [`docs/`](docs/) — see [`CONTEXT-MAP.md`](CONTEXT-MAP.md) for where to start.

## Origin story

Renderit began as a personal project to learn real-time graphics programming from first principles — a C++/OpenGL renderer built up piece by piece, from a triangle to physically based rendering and shadow mapping. That original project is preserved in full under [`educational/`](educational/README.md), roadmap, architecture notes, and all. The Three.js web viewer and this VS Code extension grew out of wanting a fast, zero-install way to actually look at the models being used to test that renderer — and turned out to be useful enough on their own to become the main project.

## License

[MIT](LICENSE)
