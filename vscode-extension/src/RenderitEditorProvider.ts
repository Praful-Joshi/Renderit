import * as vscode from "vscode";
import { getNonce, getWebviewHtml } from "./getWebviewHtml";

/** No in-memory content of its own — the model's bytes are read fresh and
 * relayed to the webview in resolveCustomEditor, since the webview sandbox
 * can't read the filesystem itself (see docs/research/vscode-webview-constraints.md §3). */
class RenderitDocument implements vscode.CustomDocument {
  constructor(readonly uri: vscode.Uri) {}
  dispose(): void {}
}

/**
 * Registers Renderit as the default editor (package.json's `customEditors`
 * contribution, `priority: "default"`) for the single-file model formats
 * (glTF/GLB, OBJ, FBX, STL, Collada) — single/double-clicking one of these
 * in the Explorer opens it directly, per issue #27/#28. `CustomReadonlyEditorProvider`
 * (not the read-write variant) since this is a viewer, not an editor — there's
 * nothing to save.
 */
export class RenderitEditorProvider implements vscode.CustomReadonlyEditorProvider<RenderitDocument> {
  static readonly viewType = "renderit.viewer";

  constructor(private readonly extensionUri: vscode.Uri) {}

  openCustomDocument(uri: vscode.Uri): RenderitDocument {
    return new RenderitDocument(uri);
  }

  async resolveCustomEditor(document: RenderitDocument, webviewPanel: vscode.WebviewPanel): Promise<void> {
    const distUri = vscode.Uri.joinPath(this.extensionUri, "dist");
    webviewPanel.webview.options = {
      enableScripts: true,
      localResourceRoots: [distUri],
    };

    const webview = webviewPanel.webview;
    const nonce = getNonce();
    webview.html = getWebviewHtml({
      scriptUri: webview.asWebviewUri(vscode.Uri.joinPath(distUri, "main.js")).toString(),
      styleUri: webview.asWebviewUri(vscode.Uri.joinPath(distUri, "main.css")).toString(),
      assetBaseUri: webview.asWebviewUri(distUri).toString(),
      cspSource: webview.cspSource,
      nonce,
    });

    // The webview signals readiness once its message listener is attached —
    // postMessage delivery to a not-yet-listening webview is best-effort, not
    // guaranteed (docs/research/vscode-webview-constraints.md §3), so the
    // host waits for this handshake rather than posting immediately.
    const subscription = webview.onDidReceiveMessage(async (message: { type?: string }) => {
      if (message?.type !== "ready") return;
      await this.postModelFile(document.uri, webview);
    });
    webviewPanel.onDidDispose(() => subscription.dispose());
  }

  private async postModelFile(uri: vscode.Uri, webview: vscode.Webview): Promise<void> {
    // Rewrapped in a fresh Uint8Array — workspace.fs.readFile's result is a
    // Buffer in practice despite its Uint8Array typing, and only a "real"
    // Uint8Array gets postMessage's fast ArrayBuffer transfer path (requires
    // engines.vscode >=1.57, declared in package.json) rather than silently
    // falling back to ~10x-slower serialization — see
    // docs/research/vscode-webview-constraints.md §4.
    const bytes = new Uint8Array(await vscode.workspace.fs.readFile(uri));
    const fileName = uri.path.split("/").pop() ?? uri.path;
    await webview.postMessage({ type: "openFile", fileName, bytes });
  }
}
