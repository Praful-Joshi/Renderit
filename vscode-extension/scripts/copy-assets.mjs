// Vite's build only bundles what webview/main.ts imports (JS/CSS) — the two
// bundled HDRI environment maps are fetched at runtime instead (see
// LightingPresetManager's injected loader in webview/main.ts), so they need
// to land in dist/ as plain files rather than going through the bundler.
import { cpSync, mkdirSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { dirname, join } from "node:path";

const here = dirname(fileURLToPath(import.meta.url));
const source = join(here, "..", "..", "web", "public", "environments");
const destination = join(here, "..", "dist", "environments");

mkdirSync(destination, { recursive: true });
cpSync(source, destination, { recursive: true });
