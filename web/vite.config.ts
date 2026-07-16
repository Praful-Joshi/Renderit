import { defineConfig } from "vitest/config";

export default defineConfig({
  // Relative base so the built site works whether it's served from a custom
  // domain, a GitHub Pages user site, or a project site subpath.
  base: "./",
  test: {
    environment: "jsdom",
    setupFiles: ["./src/test/setup.ts"],
  },
});
