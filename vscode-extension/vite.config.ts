import { defineConfig } from "vitest/config";

export default defineConfig({
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
