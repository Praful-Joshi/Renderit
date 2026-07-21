import * as vscode from "vscode";
import { onRenderitWebviewReady, readFileBytes, renderRenderitWebview, uriBasename } from "./renderitWebview";

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
    renderRenderitWebview(webviewPanel.webview, this.extensionUri);

    const subscription = onRenderitWebviewReady(webviewPanel.webview, async () => {
      const bytes = await readFileBytes(document.uri);
      await webviewPanel.webview.postMessage({ type: "openFile", fileName: uriBasename(document.uri), bytes });
    });
    webviewPanel.onDidDispose(() => subscription.dispose());
  }
}
