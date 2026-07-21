import * as assert from "node:assert";
import * as path from "node:path";
import * as vscode from "vscode";

// Runs inside a real (headless) Extension Development Host via
// @vscode/test-electron — this is the level at which VS Code extension
// behavior is actually verifiable end-to-end: activation, the customEditors
// contribution point, and RenderitEditorProvider's registration all have to
// be wired correctly for these to pass, not just typecheck. What this can't
// verify is inside the webview's own script context (model actually
// rendered, sliders work) — see the PR description for the manual
// Extension Development Host pass that covers that.
suite("Renderit custom editor", () => {
  const fixturesDir = path.join(__dirname, "..", "..", "test", "fixtures");
  const fixturesByFormat: Record<string, string> = {
    "glTF/GLB": "fixture-simple-box.glb",
    OBJ: "fixture-box.obj",
    FBX: "fixture-box-zup.fbx",
    STL: "fixture-box.stl",
    Collada: "fixture-box-zup.dae",
  };

  teardown(async () => {
    await vscode.commands.executeCommand("workbench.action.closeActiveEditor");
  });

  for (const [format, fileName] of Object.entries(fixturesByFormat)) {
    test(`opens a ${format} file (${fileName}) as the renderit.viewer custom editor`, async () => {
      const uri = vscode.Uri.file(path.join(fixturesDir, fileName));

      await vscode.commands.executeCommand("vscode.openWith", uri, "renderit.viewer");

      const activeTab = vscode.window.tabGroups.activeTabGroup.activeTab;
      assert.ok(activeTab, `expected an active tab after opening ${fileName}`);
      assert.ok(
        activeTab.input instanceof vscode.TabInputCustom,
        `expected ${fileName} to open as a custom editor tab, got ${JSON.stringify(activeTab.input)}`,
      );
      assert.strictEqual((activeTab.input as vscode.TabInputCustom).viewType, "renderit.viewer");
    });
  }
});
