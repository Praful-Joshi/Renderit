/**
 * Plain file drops give a flat FileList with no folder structure. A dropped
 * *folder* only exposes that structure through the older
 * DataTransferItemList/FileSystemEntry API — walk it recursively and stamp
 * each resulting File with a .webkitRelativePath, so it lands in
 * buildImportFileSet exactly like a folder picked via <input webkitdirectory>.
 */
export async function filesFromDataTransfer(dataTransfer: DataTransfer): Promise<File[]> {
  const entries = dataTransfer.items
    ? Array.from(dataTransfer.items)
        .map((item) => item.webkitGetAsEntry?.())
        .filter((entry): entry is FileSystemEntry => entry !== null && entry !== undefined)
    : [];

  if (entries.length === 0) {
    return Array.from(dataTransfer.files);
  }

  const files: File[] = [];
  for (const entry of entries) {
    await collectEntry(entry, "", files);
  }
  return files;
}

async function collectEntry(entry: FileSystemEntry, relativePathPrefix: string, out: File[]): Promise<void> {
  if (entry.isFile) {
    const file = await new Promise<File>((resolve, reject) => (entry as FileSystemFileEntry).file(resolve, reject));
    if (relativePathPrefix) {
      Object.defineProperty(file, "webkitRelativePath", { value: `${relativePathPrefix}${entry.name}` });
    }
    out.push(file);
    return;
  }

  if (entry.isDirectory) {
    const reader = (entry as FileSystemDirectoryEntry).createReader();
    const children = await readAllDirectoryEntries(reader);
    for (const child of children) {
      await collectEntry(child, `${relativePathPrefix}${entry.name}/`, out);
    }
  }
}

/** FileSystemDirectoryReader.readEntries() isn't guaranteed to return every
 * child in one call — the spec requires calling it repeatedly until it
 * returns an empty batch. */
function readAllDirectoryEntries(reader: FileSystemDirectoryReader): Promise<FileSystemEntry[]> {
  return new Promise((resolve, reject) => {
    const all: FileSystemEntry[] = [];
    const readBatch = () => {
      reader.readEntries((batch) => {
        if (batch.length === 0) {
          resolve(all);
        } else {
          all.push(...batch);
          readBatch();
        }
      }, reject);
    };
    readBatch();
  });
}
