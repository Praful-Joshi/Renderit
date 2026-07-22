import { describe, expect, it } from "vitest";
import { getNonce, getWebviewHtml } from "./getWebviewHtml";

const options = {
  scriptUri: "https://fake-uuid.vscode-webview.net/main.js",
  styleUri: "https://fake-uuid.vscode-webview.net/style.css",
  cspSource: "https://fake-uuid.vscode-webview.net",
  nonce: "test-nonce-value",
  assetBaseUri: "https://fake-uuid.vscode-webview.net/dist",
};

describe("getWebviewHtml", () => {
  it("scopes the CSP to the webview's own cspSource and the script's nonce", () => {
    const html = getWebviewHtml(options);
    expect(html).toContain(`img-src ${options.cspSource}`);
    expect(html).toContain(`connect-src ${options.cspSource}`);
    expect(html).toContain(`style-src ${options.cspSource}`);
    expect(html).toContain(`script-src 'nonce-${options.nonce}'`);
  });

  it("allows blob:/data: image sources — ResourceResolver.ts resolves textures via URL.createObjectURL", () => {
    const html = getWebviewHtml(options);
    expect(html).toMatch(/img-src[^;]*\bblob:/);
    expect(html).toMatch(/img-src[^;]*\bdata:/);
  });

  // Regression: an earlier version only allowlisted `connect-src ${cspSource}`,
  // which silently blocked every fetch() call GLTFLoader/THREE's
  // LoadingManager make to load a buffer/texture referenced by blob: (an
  // external file resolved via ResourceResolver's createObjectURL) or data:
  // (an embedded base64 buffer) — manifesting as "Failed to load buffer"
  // for any multi-file model or one with an embedded data URI, with no CSP
  // violation surfaced to the user (only single-file, fully self-contained
  // binary formats like .glb happened to avoid the extra fetch and render).
  it("allows blob:/data: fetches — GLTFLoader fetches referenced buffers/textures by URL, not just <img> src", () => {
    const html = getWebviewHtml(options);
    expect(html).toMatch(/connect-src[^;]*\bblob:/);
    expect(html).toMatch(/connect-src[^;]*\bdata:/);
  });

  // Regression: without an explicit worker-src, it falls back to script-src
  // (nonce-only) — fflate's async unzip() (ImportFileSet.ts's extractZip)
  // creates a Worker from a blob: URL to decompress off the main thread,
  // which CSP silently blocked with no catchable JS error (just a console
  // warning), hanging forever on any zip large enough for fflate to bother
  // using a worker at all. A nonce can't apply to worker construction the
  // way it does to a <script> tag, so this needs its own directive.
  it("allows blob: workers — fflate's async unzip() decompresses off the main thread via a blob: Worker", () => {
    const html = getWebviewHtml(options);
    expect(html).toMatch(/worker-src[^;]*\bblob:/);
  });

  it("never allows unsafe-inline/unsafe-eval scripts", () => {
    const html = getWebviewHtml(options);
    expect(html).not.toContain("unsafe-inline");
    expect(html).not.toContain("unsafe-eval");
  });

  it("points the script and stylesheet tags at the resolved asWebviewUri values", () => {
    const html = getWebviewHtml(options);
    expect(html).toContain(`<script type="module" nonce="${options.nonce}" src="${options.scriptUri}"></script>`);
    expect(html).toContain(`<link rel="stylesheet" href="${options.styleUri}" />`);
  });

  it("injects the asset base URI as a nonce'd inline script so LightingPresets can load HDRIs synchronously at bootstrap", () => {
    const html = getWebviewHtml(options);
    expect(html).toContain(`<script nonce="${options.nonce}">`);
    expect(html).toContain(`window.__RENDERIT_ASSET_BASE_URI__ = ${JSON.stringify(options.assetBaseUri)};`);
  });

  it("has no welcome-page/import-button UI — opening is Explorer-native, not in-webview (issue #27)", () => {
    const html = getWebviewHtml(options);
    expect(html).not.toContain("browse-button");
    expect(html).not.toContain("browse-folder-button");
    expect(html).not.toContain("file-input");
    expect(html).not.toContain("import-overlay");
  });

  it("includes the settings and metadata panel controls the webview bootstrap wires up", () => {
    const html = getWebviewHtml(options);
    for (const id of [
      "viewer-canvas",
      "scale-slider",
      "rotation-x-slider",
      "rotation-y-slider",
      "rotation-z-slider",
      "lighting-preset-day-button",
      "lighting-preset-night-button",
      "background-mode-studio-button",
      "background-mode-hdri-button",
      "reset-view-button",
      "metadata-file-name",
    ]) {
      expect(html).toContain(`id="${id}"`);
    }
  });
});

describe("getNonce", () => {
  it("returns a 32-character alphanumeric string", () => {
    expect(getNonce()).toMatch(/^[A-Za-z0-9]{32}$/);
  });

  it("returns a different value on each call", () => {
    expect(getNonce()).not.toBe(getNonce());
  });
});
