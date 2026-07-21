/** Kept free of any `vscode` import — callers resolve the actual
 * `asWebviewUri`/`cspSource` values first, so this stays a pure string
 * template testable without a real extension host (see
 * docs/research/vscode-webview-constraints.md §1, §7). */
export interface WebviewHtmlOptions {
  scriptUri: string;
  styleUri: string;
  cspSource: string;
  nonce: string;
  /** asWebviewUri-mapped base for the bundled static assets (the HDRI
   * environment maps) — injected synchronously as a global rather than
   * relayed over postMessage, since (unlike the opened model's bytes) it
   * doesn't require an async host-side read; see webview/main.ts. */
  assetBaseUri: string;
}

/**
 * Hand-authored webview shell (not Vite's own dist/index.html — see
 * docs/research/vscode-webview-constraints.md §7 for why). Mirrors
 * web/index.html's viewport + sidebar markup minus the welcome-page/import
 * UI (browse buttons, file inputs, drag overlay) — this extension has no
 * in-webview import flow, opening is Explorer-native (see issue #27).
 */
export function getWebviewHtml({ scriptUri, styleUri, cspSource, nonce, assetBaseUri }: WebviewHtmlOptions): string {
  return `<!doctype html>
<html lang="en">
  <head>
    <meta charset="UTF-8" />
    <meta
      http-equiv="Content-Security-Policy"
      content="default-src 'none'; img-src ${cspSource} blob: data:; connect-src ${cspSource}; script-src 'nonce-${nonce}'; style-src ${cspSource};"
    />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <title>Renderit</title>
    <link rel="stylesheet" href="${styleUri}" />
  </head>
  <body>
    <div id="app">
      <main id="viewport-region">
        <canvas id="viewer-canvas"></canvas>
        <p id="import-error" role="alert" hidden></p>
      </main>
      <aside id="sidebar-region">
        <h1 class="brand">Renderit</h1>
        <section id="settings-panel" class="sidebar-panel">
          <h2>Settings</h2>
          <div id="scale-control" class="settings-slider">
            <div class="settings-slider-header">
              <label for="scale-slider">Scale</label>
              <output id="scale-value" for="scale-slider">1.00</output>
            </div>
            <input id="scale-slider" type="range" min="0.1" max="3" step="0.01" value="1" />
          </div>
          <div id="rotation-x-control" class="settings-slider">
            <div class="settings-slider-header">
              <label for="rotation-x-slider">Rotation X</label>
              <output id="rotation-x-value" for="rotation-x-slider">0°</output>
            </div>
            <input id="rotation-x-slider" type="range" min="-180" max="180" step="1" value="0" />
          </div>
          <div id="rotation-y-control" class="settings-slider">
            <div class="settings-slider-header">
              <label for="rotation-y-slider">Rotation Y</label>
              <output id="rotation-y-value" for="rotation-y-slider">0°</output>
            </div>
            <input id="rotation-y-slider" type="range" min="-180" max="180" step="1" value="0" />
          </div>
          <div id="rotation-z-control" class="settings-slider">
            <div class="settings-slider-header">
              <label for="rotation-z-slider">Rotation Z</label>
              <output id="rotation-z-value" for="rotation-z-slider">0°</output>
            </div>
            <input id="rotation-z-slider" type="range" min="-180" max="180" step="1" value="0" />
          </div>
          <div id="lighting-preset-toggle" class="settings-toggle" role="group" aria-label="Lighting preset">
            <button id="lighting-preset-day-button" type="button" aria-pressed="true">Day</button>
            <button id="lighting-preset-night-button" type="button" aria-pressed="false">Night</button>
          </div>
          <div id="background-mode-toggle" class="settings-toggle" role="group" aria-label="Background mode">
            <button id="background-mode-studio-button" type="button" aria-pressed="true">Studio</button>
            <button id="background-mode-hdri-button" type="button" aria-pressed="false">HDRI</button>
          </div>
          <button id="reset-view-button" type="button">Reset view</button>
        </section>
        <section id="metadata-panel" class="sidebar-panel">
          <h2>Metadata</h2>
          <dl id="metadata-list">
            <div class="metadata-row"><dt>File</dt><dd id="metadata-file-name">—</dd></div>
            <div class="metadata-row"><dt>Format</dt><dd id="metadata-format">—</dd></div>
            <div class="metadata-row"><dt>Size</dt><dd id="metadata-file-size">—</dd></div>
            <div class="metadata-row"><dt>Triangles</dt><dd id="metadata-triangle-count">—</dd></div>
            <div class="metadata-row"><dt>Vertices</dt><dd id="metadata-vertex-count">—</dd></div>
            <div class="metadata-row"><dt>Meshes</dt><dd id="metadata-mesh-count">—</dd></div>
            <div class="metadata-row"><dt>Materials</dt><dd id="metadata-material-count">—</dd></div>
            <div class="metadata-row"><dt>Textures</dt><dd id="metadata-texture-count">—</dd></div>
            <div class="metadata-row"><dt>Dimensions</dt><dd id="metadata-dimensions">—</dd></div>
          </dl>
        </section>
      </aside>
    </div>
    <script nonce="${nonce}">window.__RENDERIT_ASSET_BASE_URI__ = ${JSON.stringify(assetBaseUri)};</script>
    <script type="module" nonce="${nonce}" src="${scriptUri}"></script>
  </body>
</html>`;
}

/** Copied from the official `webview-sample` extension (microsoft/vscode-extension-samples) —
 * the documented nonce pattern for a webview's CSP script-src, see
 * docs/research/vscode-webview-constraints.md §1. */
export function getNonce(): string {
  let text = "";
  const possible = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
  for (let i = 0; i < 32; i++) {
    text += possible.charAt(Math.floor(Math.random() * possible.length));
  }
  return text;
}
