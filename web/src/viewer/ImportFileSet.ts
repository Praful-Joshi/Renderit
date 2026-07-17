import { unzip, type Unzipped } from "fflate";

/** All the files involved in one import, keyed by path relative to the
 * import's own root (a zip's internal root, or the selected folder's
 * contents) — never including a container folder's own name. Plain files
 * (single file, or a flat multi-select) key by their bare filename. */
export type ImportFileSet = Map<string, File>;

export async function buildImportFileSet(source: File | File[]): Promise<ImportFileSet> {
  // Normalize before the zip check — a zip picked through a `multiple`
  // file input arrives as a one-element array, not a bare File, so the
  // check has to cover both shapes or a picked (as opposed to dropped) zip
  // silently skips decompression entirely.
  const files = Array.isArray(source) ? source : [source];

  if (files.length === 1 && files[0].name.toLowerCase().endsWith(".zip")) {
    return extractZip(files[0]);
  }

  const entries = files.map((file) => ({ file, path: file.webkitRelativePath || file.name }));
  stripCommonRootSegment(entries);

  return new Map(entries.map(({ file, path }) => [path, file]));
}

async function extractZip(zipFile: File): Promise<ImportFileSet> {
  const buffer = new Uint8Array(await zipFile.arrayBuffer());
  const entries = await new Promise<Unzipped>((resolve, reject) => {
    unzip(buffer, (err, data) => (err ? reject(err) : resolve(data)));
  });

  const fileSet: ImportFileSet = new Map();
  for (const [path, bytes] of Object.entries(entries)) {
    if (path.endsWith("/")) continue; // directory entry, not a file
    const basename = path.split("/").pop() ?? path;
    fileSet.set(path, new File([bytes], basename));
  }
  return fileSet;
}

/** Folder-picker/drag-drop paths all start with the same container folder
 * name (the folder the visitor selected) — strip it so keys match how a
 * model's own internal references are authored (relative to its contents,
 * not to whatever the visitor happened to name the folder). Only strips
 * when every relPath-bearing entry agrees on that first segment. */
function stripCommonRootSegment(entries: { file: File; path: string }[]): void {
  const withRelPath = entries.filter((entry) => entry.file.webkitRelativePath);
  if (withRelPath.length === 0) return;

  const firstSegments = new Set(withRelPath.map((entry) => entry.path.split("/")[0]));
  if (firstSegments.size !== 1) return;

  const prefix = `${[...firstSegments][0]}/`;
  for (const entry of withRelPath) {
    entry.path = entry.path.slice(prefix.length);
  }
}
