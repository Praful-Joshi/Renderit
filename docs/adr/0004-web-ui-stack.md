# Web viewer UI: Vite + TypeScript + vanilla DOM, no UI framework

The settings sidebar, metadata panel, and drag-drop surface need to be built somehow. React (optionally with `react-three-fiber`) was considered — it's the default assumption for most web projects and would give more structure if the UI grows substantially.

**Decision:** vanilla TypeScript with plain DOM/CSS, no framework. The UI surface here is genuinely modest for v1 (a handful of sliders, a theme toggle, a metadata list, a drag-drop zone) and hand-building it keeps the dependency footprint minimal, matches the project's "simple but polished" goal, and keeps direct, unabstracted control over the Three.js scene rather than routing it through a component-render cycle. Bundler is Vite; language is TypeScript throughout (pairs naturally with Three.js's typed API).

**Consequence:** if the UI grows significantly past v1 scope (e.g. the browser-extension fast-follow needs substantially different/duplicated UI, or more panels get added), revisit — this decision assumes a small, stable UI surface.
