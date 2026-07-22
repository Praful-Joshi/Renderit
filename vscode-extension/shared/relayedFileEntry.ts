/**
 * The wire shape for one file relayed from the extension host to the
 * webview over postMessage (folder import, issue #29) — shared between
 * src/openWithRenderit.ts (host, produces it) and
 * webview/filesFromRelayedEntries.ts (webview, consumes it) so the two sides
 * of the postMessage boundary can't drift apart silently. Host and webview
 * compile under separate tsconfigs (Node vs DOM/bundler), so this is the
 * only thing keeping them in sync — a type-only module has no runtime
 * footprint in either context.
 */
export interface RelayedFileEntry {
  relativePath: string;
  // Never SharedArrayBuffer-backed in practice — VS Code's postMessage
  // bridge can't transfer one at all (see
  // docs/research/vscode-webview-constraints.md §4).
  bytes: Uint8Array<ArrayBuffer>;
}
