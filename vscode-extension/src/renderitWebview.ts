import * as vscode from "vscode";
import { getNonce, getWebviewHtml } from "./getWebviewHtml";

/** Points webview.options/html at the bundled dist/ output — shared between
 * RenderitEditorProvider (single-file open, issue #28) and openWithRenderit
 * (zip/folder open, issue #29), since both load the identical webview bundle
 * with identical CSP/asWebviewUri/nonce wiring, just triggered differently. */
export function renderRenderitWebview(webview: vscode.Webview, extensionUri: vscode.Uri): void {
  const distUri = vscode.Uri.joinPath(extensionUri, "dist");
  webview.options = {
    enableScripts: true,
    localResourceRoots: [distUri],
  };

  webview.html = getWebviewHtml({
    scriptUri: webview.asWebviewUri(vscode.Uri.joinPath(distUri, "main.js")).toString(),
    styleUri: webview.asWebviewUri(vscode.Uri.joinPath(distUri, "main.css")).toString(),
    assetBaseUri: webview.asWebviewUri(distUri).toString(),
    cspSource: webview.cspSource,
    nonce: getNonce(),
  });
}

/** Runs `onReady` once the webview signals its message listener is attached —
 * postMessage delivery to a not-yet-listening webview is best-effort, not
 * guaranteed (docs/research/vscode-webview-constraints.md §3), so callers
 * must wait for this handshake rather than posting right after the webview
 * is created. */
export function onRenderitWebviewReady(webview: vscode.Webview, onReady: () => Promise<void>): vscode.Disposable {
  return webview.onDidReceiveMessage(async (message: { type?: string }) => {
    if (message?.type !== "ready") return;
    await onReady();
  });
}

/** Rewrapped in a fresh Uint8Array — workspace.fs.readFile's result is a
 * Buffer in practice despite its Uint8Array typing, and only a "real"
 * Uint8Array gets postMessage's fast ArrayBuffer transfer path (requires
 * engines.vscode >=1.57, declared in package.json) rather than silently
 * falling back to ~10x-slower serialization — see
 * docs/research/vscode-webview-constraints.md §4. */
export async function readFileBytes(uri: vscode.Uri): Promise<Uint8Array<ArrayBuffer>> {
  return new Uint8Array(await vscode.workspace.fs.readFile(uri));
}

export function uriBasename(uri: vscode.Uri): string {
  return uri.path.split("/").pop() ?? uri.path;
}
