import { fileURLToPath } from "node:url";
import { defineConfig } from "vitest/config";

const here = fileURLToPath(new URL(".", import.meta.url));

export default defineConfig({
  // web/src/viewer/*.ts's own bare "three" import resolves relative to
  // *that file's* location — since web/ is a sibling project with its own
  // node_modules/three, Node's resolution algorithm finds web/node_modules/
  // three there, a physically different copy from this project's own
  // node_modules/three (same version, still two separate module instances
  // to the JS runtime — THREE.js itself warns about this, and it can break
  // instanceof checks). Force every "three" import, regardless of which
  // file requests it, to resolve to this project's single copy.
  resolve: {
    // three's own package.json "exports" maps "." to build/three.module.js
    // and "./addons/*" to examples/jsm/* (there's no literal addons/
    // directory on disk) — replicated explicitly here rather than aliasing
    // to the bare "node_modules/three" directory, since a plain path
    // replacement bypasses the exports map and 404s on addon subpaths.
    // Order matters: the subpath rule must come first, or GLTFLoader.js et
    // al. get loaded from web/node_modules/three/examples/jsm/... and their
    // own internal relative imports pull in web/'s copy of the core module
    // regardless of the bare "three" alias below.
    alias: [
      { find: /^three\/addons\//, replacement: `${here}node_modules/three/examples/jsm/` },
      { find: /^three$/, replacement: `${here}node_modules/three/build/three.module.js` },
    ],
  },
  // Relative base + fixed (non-hashed) output filenames: the extension host
  // hand-templates the webview's HTML at activation time (see
  // src/getWebviewHtml.ts) and needs to reference the bundle's script/style
  // URIs by a known name, not a Vite-generated content hash — see
  // docs/research/vscode-webview-constraints.md §7.
  base: "./",
  build: {
    outDir: "dist",
    emptyOutDir: true,
    rollupOptions: {
      input: "webview/main.ts",
      output: {
        entryFileNames: "main.js",
        chunkFileNames: "[name].js",
        assetFileNames: "[name][extname]",
      },
    },
  },
  test: {
    environment: "jsdom",
    include: ["src/**/*.test.ts", "webview/**/*.test.ts"],
  },
});
