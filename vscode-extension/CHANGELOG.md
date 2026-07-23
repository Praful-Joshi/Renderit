# Changelog

All notable changes to the Renderit VS Code extension are documented here.
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

## [0.1.0] - 2026-07-22

Initial release.

### Added

- Single/double-click open for `.gltf`, `.glb`, `.obj`, `.fbx`, `.stl`, and `.dae` files directly in a Renderit editor tab, via a `CustomEditorProvider` registered as the default handler for those extensions.
- Right-click **"Open with Renderit"** on a `.zip` file or a folder in the Explorer, resolving flat or nested texture layouts the same way the [web viewer](https://github.com/Praful-Joshi/Renderit/tree/main/web) already does.
- Orbit camera with damping, scroll/trackpad zoom, idle auto-rotate until first interaction, and a reset-view button.
- Scale and per-axis (X/Y/Z) rotation sliders.
- Day/Night lighting presets and Studio/HDRI background modes.
- Metadata panel: file name, format, size, triangle/vertex count, mesh/material/texture counts, bounding-box dimensions.
- State (camera pose, scale, rotation, lighting preset, background mode) survives switching away from and back to a Renderit tab.
- Clear error messaging for unsupported file types and unresolvable texture references.

### Known limitations

- No progress indicator while a large model is being decompressed/parsed — the UI briefly appears idle rather than showing it's working.
- Does not yet survive a full VS Code window restart (the tab has to still be open in the same session) — tracked separately from the tab-switch persistence above.
