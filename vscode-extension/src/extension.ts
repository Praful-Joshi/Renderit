import * as vscode from "vscode";
import { RenderitEditorProvider } from "./RenderitEditorProvider";

export function activate(context: vscode.ExtensionContext): void {
  context.subscriptions.push(
    vscode.window.registerCustomEditorProvider(
      RenderitEditorProvider.viewType,
      new RenderitEditorProvider(context.extensionUri),
      { webviewOptions: { retainContextWhenHidden: false } },
    ),
  );
}

export function deactivate(): void {}
