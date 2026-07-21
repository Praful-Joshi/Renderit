import * as assert from "node:assert";
import * as path from "node:path";
import * as vscode from "vscode";
import { collectFolderFiles } from "../src/openWithRenderit";

// .vscode-test.mjs points the test run's workspace at test/fixtures/ itself,
// so this is the fixtures directory regardless of where tsc happens to place
// the compiled test file (__dirname-relative arithmetic is fragile — it
// silently broke once when rootDir changed depth; this isn't).
function fixturesDir(): string {
  const [workspaceFolder] = vscode.workspace.workspaceFolders ?? [];
  if (!workspaceFolder) throw new Error("Expected a workspace folder (see .vscode-test.mjs's workspaceFolder)");
  return workspaceFolder.uri.fsPath;
}

/** Polls until `predicate` is true or `timeoutMs` elapses — createWebviewPanel's
 * own promise resolves before VS Code's tab model necessarily reflects the
 * new tab, and a single onDidChangeTabs event is ambiguous (the previous
 * test's teardown-triggered close can fire its own delayed event right as the
 * next test's tab appears), so polling the actual condition is more robust
 * than reacting to one event. */
async function waitUntil(predicate: () => boolean, timeoutMs = 3000, intervalMs = 25): Promise<void> {
  const deadline = Date.now() + timeoutMs;
  while (!predicate()) {
    if (Date.now() > deadline) throw new Error(`Timed out after ${timeoutMs}ms waiting for condition`);
    await new Promise((resolve) => setTimeout(resolve, intervalMs));
  }
}

// Runs inside a real (headless) Extension Development Host via
// @vscode/test-electron — this is the level at which VS Code extension
// behavior is actually verifiable end-to-end: activation, the customEditors
// contribution point, and RenderitEditorProvider's registration all have to
// be wired correctly for these to pass, not just typecheck. What this can't
// verify is inside the webview's own script context (model actually
// rendered, sliders work) — see the PR description for the manual
// Extension Development Host pass that covers that.
suite("Renderit custom editor", () => {
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
      const uri = vscode.Uri.file(path.join(fixturesDir(), fileName));

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

// Issue #29 — "Open with Renderit" (explorer/context command) for zip/folder
// import. createWebviewPanel's returned promise resolves right after the
// panel/tab is created, before the async ready-handshake (webview loads,
// signals "ready", host reads+relays bytes) completes — so these tests can
// verify the tab appears and the command doesn't throw host-side, but can't
// observe what happens inside the webview's own script context (whether the
// right warning/error text is shown, whether textures actually resolved).
// That boundary is the same one #28's custom-editor tests hit; see the PR
// description for what was verified manually instead.
suite("Renderit 'Open with Renderit' command", () => {
  teardown(async () => {
    await vscode.commands.executeCommand("workbench.action.closeActiveEditor");
  });

  test("registers the renderit.openWithRenderit command", async () => {
    const commands = await vscode.commands.getCommands(true);
    assert.ok(commands.includes("renderit.openWithRenderit"));
  });

  const zipAndFolderCases: Record<string, string> = {
    "a flat zip": "fixture-model-flat.zip",
    "a nested zip": "fixture-model-nested.zip",
    "a flat folder": "folder-flat",
    "a nested folder": "folder-nested",
    "a zip with an unresolvable texture reference": "fixture-missing-texture.zip",
    "a folder with an unresolvable texture reference": "folder-missing-texture",
    "a zip with an unsupported file format": "fixture-unsupported-format.zip",
    "a folder with an unsupported file format": "folder-unsupported-format",
  };

  for (const [description, fixtureName] of Object.entries(zipAndFolderCases)) {
    test(`opens a new Renderit tab for ${description} without throwing`, async () => {
      const uri = vscode.Uri.file(path.join(fixturesDir(), fixtureName));

      await vscode.commands.executeCommand("renderit.openWithRenderit", uri);
      // createWebviewPanel's synchronous return doesn't mean
      // vscode.window.tabGroups already reflects the new tab — VS Code
      // updates that model on a later tick.
      await waitUntil(() => vscode.window.tabGroups.activeTabGroup.activeTab?.input instanceof vscode.TabInputWebview);

      const activeTab = vscode.window.tabGroups.activeTabGroup.activeTab;
      assert.ok(activeTab, `expected a new tab after opening ${fixtureName}`);
      assert.ok(
        activeTab.input instanceof vscode.TabInputWebview,
        `expected ${fixtureName} to open as a webview tab, got ${JSON.stringify(activeTab.input)}`,
      );
    });
  }

  // #29's "spike and document" acceptance criterion: measures the host-side
  // recursive-read cost directly (the part under this extension's own
  // control, and the most plausible place a large import could hang) against
  // a ~10MB/6-file fixture — see the PR description for the observed number.
  test("collects a ~10MB/6-file folder without hanging (spike measurement)", async () => {
    const uri = vscode.Uri.file(path.join(fixturesDir(), "folder-large-payload"));

    const start = Date.now();
    const files = await collectFolderFiles(uri);
    const elapsedMs = Date.now() - start;

    const totalBytes = files.reduce((sum, file) => sum + file.bytes.byteLength, 0);
    console.log(`[spike] collectFolderFiles: ${files.length} files, ${totalBytes} bytes, ${elapsedMs}ms`);

    assert.strictEqual(files.length, 6);
    assert.ok(totalBytes > 9 * 1024 * 1024, `expected >9MB total, got ${totalBytes} bytes`);
    assert.ok(elapsedMs < 5000, `expected the folder walk to finish in under 5s, took ${elapsedMs}ms`);
  });
});
