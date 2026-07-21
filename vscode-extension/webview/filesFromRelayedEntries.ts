import type { RelayedFileEntry } from "../shared/relayedFileEntry";

export type { RelayedFileEntry };

/**
 * Reconstructs File objects with .webkitRelativePath stamped to match what
 * ImportFileSet.ts already expects from a real folder-picker/OS drag-drop —
 * the host-side folder walk (src/openWithRenderit.ts) produces the same
 * "{folderName}/{subpath}" shape, so ImportFileSet's existing
 * stripCommonRootSegment (unchanged) strips the prefix identically to how it
 * already does for the website. See docs/research/vscode-webview-constraints.md §8.
 */
export function filesFromRelayedEntries(entries: RelayedFileEntry[]): File[] {
  return entries.map(({ relativePath, bytes }) => {
    const name = relativePath.split("/").pop() ?? relativePath;
    const file = new File([bytes], name);
    Object.defineProperty(file, "webkitRelativePath", { value: relativePath });
    return file;
  });
}
