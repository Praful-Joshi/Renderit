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

  it("allows blob: image sources — ResourceResolver.ts resolves textures via URL.createObjectURL", () => {
    const html = getWebviewHtml(options);
    expect(html).toMatch(/img-src[^;]*\bblob:/);
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
