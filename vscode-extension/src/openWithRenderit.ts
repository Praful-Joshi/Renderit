import * as vscode from "vscode";
import { RenderitEditorProvider } from "./RenderitEditorProvider";
import { onRenderitWebviewReady, readFileBytes, renderRenderitWebview, uriBasename } from "./renderitWebview";
import type { RelayedFileEntry } from "../shared/relayedFileEntry";

/**
 * The Explorer right-click entry point for zip/folder import (issue #29) —
 * custom editors are file-resource-only with no folder equivalent, so this
 * is a plain command (contributed to `explorer/context`, gated by
 * `explorerResourceIsFolder || resourceExtname == .zip`) rather than a
 * second CustomEditorProvider. See docs/research/vscode-webview-constraints.md §8.
 *
 * Opens a *new* webview panel per invocation, reusing the same bundle/HTML
 * shell as RenderitEditorProvider (issue #28) — not the same panel, per #29's
 * own scope ("doesn't need to reuse an already-open panel").
 */
export function registerOpenWithRenderitCommand(extensionUri: vscode.Uri): vscode.Disposable {
  return vscode.commands.registerCommand("renderit.openWithRenderit", async (uri: vscode.Uri) => {
    const panel = vscode.window.createWebviewPanel(
      RenderitEditorProvider.viewType,
      uriBasename(uri),
      vscode.ViewColumn.Active,
      { retainContextWhenHidden: false },
    );
    renderRenderitWebview(panel.webview, extensionUri);

    const subscription = onRenderitWebviewReady(panel.webview, async () => {
      const stat = await vscode.workspace.fs.stat(uri);
      if (stat.type === vscode.FileType.Directory) {
        const files = await collectFolderFiles(uri);
        await panel.webview.postMessage({ type: "openFiles", files });
      } else {
        // A zip is relayed as a single file, reusing the exact same message
        // shape/webview handler as the single-file open path (#28) —
        // ImportFileSet.ts's own zip detection (by ".zip" extension) and
        // fflate-based extraction already run entirely client-side inside
        // the webview, completely unmodified.
        const bytes = await readFileBytes(uri);
        await panel.webview.postMessage({ type: "openFile", fileName: uriBasename(uri), bytes });
      }
    });
    panel.onDidDispose(() => subscription.dispose());
  });
}

/** Walks a folder recursively, producing the same "{folderName}/{subpath}"
 * relative-path shape a real `<input webkitdirectory>` pick or OS drag-drop
 * folder already produces — ImportFileSet.ts's stripCommonRootSegment
 * (unchanged) strips the folder-name prefix back off exactly like it already
 * does for the website's folder import. Exported (not just used internally)
 * so the integration test can time it directly against a realistic
 * multi-megabyte fixture — see test/extension.test.ts's large-payload case
 * and #29's "spike large folder imports" acceptance criterion. */
export async function collectFolderFiles(folderUri: vscode.Uri): Promise<RelayedFileEntry[]> {
  const folderName = uriBasename(folderUri);
  const files: RelayedFileEntry[] = [];

  async function walk(dirUri: vscode.Uri, prefix: string): Promise<void> {
    const entries = await vscode.workspace.fs.readDirectory(dirUri);
    for (const [name, type] of entries) {
      const childUri = vscode.Uri.joinPath(dirUri, name);
      if (type === vscode.FileType.Directory) {
        await walk(childUri, `${prefix}${name}/`);
      } else if (type === vscode.FileType.File) {
        files.push({ relativePath: `${folderName}/${prefix}${name}`, bytes: await readFileBytes(childUri) });
      }
    }
  }

  await walk(folderUri, "");
  return files;
}
