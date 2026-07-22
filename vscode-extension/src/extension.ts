import * as vscode from "vscode";
import { RenderitEditorProvider } from "./RenderitEditorProvider";
import { registerOpenWithRenderitCommand } from "./openWithRenderit";

export function activate(context: vscode.ExtensionContext): void {
  context.subscriptions.push(
    vscode.window.registerCustomEditorProvider(
      RenderitEditorProvider.viewType,
      new RenderitEditorProvider(context.extensionUri),
      { webviewOptions: { retainContextWhenHidden: false } },
    ),
    registerOpenWithRenderitCommand(context.extensionUri),
  );
}

export function deactivate(): void {}
