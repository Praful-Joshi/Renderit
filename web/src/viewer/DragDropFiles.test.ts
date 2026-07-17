import { describe, expect, it } from "vitest";
import { filesFromDataTransfer } from "./DragDropFiles";

/** Minimal fake matching the FileSystemFileEntry interface shape that
 * filesFromDataTransfer/collectEntry actually touch (.isFile, .isDirectory,
 * .name, .file()). Real drag-drop entries can't be constructed in a test
 * environment (no OS-level drop), so this is the standard way to test
 * FileSystemEntry-walking logic in isolation. */
function fakeFileEntry(name: string, content = "x"): FileSystemEntry {
  return {
    isFile: true,
    isDirectory: false,
    name,
    file(successCallback: (file: File) => void) {
      successCallback(new File([content], name));
    },
  } as unknown as FileSystemEntry;
}

/** Fake FileSystemDirectoryEntry. `readEntries` is called repeatedly by the
 * real API until it returns an empty batch — this fake returns everything
 * in one batch, then empty, matching that contract. */
function fakeDirectoryEntry(name: string, children: FileSystemEntry[]): FileSystemEntry {
  let delivered = false;
  return {
    isFile: false,
    isDirectory: true,
    name,
    createReader() {
      return {
        readEntries(successCallback: (entries: FileSystemEntry[]) => void) {
          if (delivered) {
            successCallback([]);
          } else {
            delivered = true;
            successCallback(children);
          }
        },
      };
    },
  } as unknown as FileSystemEntry;
}

function fakeDataTransfer(entries: FileSystemEntry[]): DataTransfer {
  return {
    items: entries.map((entry) => ({ webkitGetAsEntry: () => entry })),
    files: [],
  } as unknown as DataTransfer;
}

describe("filesFromDataTransfer", () => {
  it("returns loose dropped files unchanged, with no webkitRelativePath", async () => {
    const dataTransfer = fakeDataTransfer([fakeFileEntry("model.obj"), fakeFileEntry("model.mtl")]);

    const files = await filesFromDataTransfer(dataTransfer);

    expect(files.map((f) => f.name).sort()).toEqual(["model.mtl", "model.obj"]);
    expect(files.every((f) => !f.webkitRelativePath)).toBe(true);
  });

  it("walks a dropped folder recursively, stamping webkitRelativePath relative to the folder's contents", async () => {
    const dataTransfer = fakeDataTransfer([
      fakeDirectoryEntry("MyModel", [
        fakeFileEntry("model.gltf"),
        fakeDirectoryEntry("textures", [fakeFileEntry("diffuse.png")]),
      ]),
    ]);

    const files = await filesFromDataTransfer(dataTransfer);

    const paths = files.map((f) => f.webkitRelativePath).sort();
    expect(paths).toEqual(["MyModel/model.gltf", "MyModel/textures/diffuse.png"]);
  });

  it("falls back to dataTransfer.files when no items/entries are available", async () => {
    const file = new File(["x"], "model.glb");
    const dataTransfer = { items: undefined, files: [file] } as unknown as DataTransfer;

    const files = await filesFromDataTransfer(dataTransfer);

    expect(files).toEqual([file]);
  });

  it("calls readEntries repeatedly until an empty batch, not just once", async () => {
    let calls = 0;
    const entry = {
      isFile: false,
      isDirectory: true,
      name: "MyModel",
      createReader() {
        return {
          readEntries(cb: (entries: FileSystemEntry[]) => void) {
            calls++;
            if (calls === 1) cb([fakeFileEntry("a.txt")]);
            else if (calls === 2) cb([fakeFileEntry("b.txt")]);
            else cb([]);
          },
        };
      },
    } as unknown as FileSystemEntry;
    const dataTransfer = fakeDataTransfer([entry]);

    const files = await filesFromDataTransfer(dataTransfer);

    expect(calls).toBeGreaterThanOrEqual(3);
    expect(files.map((f) => f.webkitRelativePath).sort()).toEqual(["MyModel/a.txt", "MyModel/b.txt"]);
  });
});
