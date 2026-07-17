import { describe, expect, it } from "vitest";
import { zipSync } from "fflate";
import { buildImportFileSet } from "./ImportFileSet";

function makeFile(name: string, content = "x"): File {
  return new File([content], name);
}

function makeFolderFile(relativePath: string, content = "x"): File {
  const file = makeFile(relativePath.split("/").pop()!, content);
  Object.defineProperty(file, "webkitRelativePath", { value: relativePath });
  return file;
}

describe("buildImportFileSet", () => {
  it("builds a single-entry set from one File", async () => {
    const set = await buildImportFileSet(makeFile("model.glb"));
    expect([...set.keys()]).toEqual(["model.glb"]);
  });

  it("builds a flat set from an array of Files with no directory structure", async () => {
    const set = await buildImportFileSet([makeFile("model.obj"), makeFile("model.mtl")]);
    expect(new Set(set.keys())).toEqual(new Set(["model.obj", "model.mtl"]));
  });

  it("strips the common root folder from webkitRelativePath entries (folder picker, flat)", async () => {
    const set = await buildImportFileSet([
      makeFolderFile("MyModel/model.gltf"),
      makeFolderFile("MyModel/model.bin"),
    ]);
    expect(new Set(set.keys())).toEqual(new Set(["model.gltf", "model.bin"]));
  });

  it("strips the common root folder from webkitRelativePath entries (folder picker, nested)", async () => {
    const set = await buildImportFileSet([
      makeFolderFile("MyModel/source/model.gltf"),
      makeFolderFile("MyModel/textures/diffuse.png"),
    ]);
    expect(new Set(set.keys())).toEqual(new Set(["source/model.gltf", "textures/diffuse.png"]));
  });

  it("decompresses a flat zip into a file set with matching relative paths", async () => {
    const zipped = zipSync({
      "model.gltf": new TextEncoder().encode("{}"),
      "diffuse.png": new TextEncoder().encode("fake-png-bytes"),
    });
    const zipFile = new File([zipped], "model.zip");

    const set = await buildImportFileSet(zipFile);

    expect(new Set(set.keys())).toEqual(new Set(["model.gltf", "diffuse.png"]));
  });

  it("decompresses a nested zip, preserving subfolder paths", async () => {
    const zipped = zipSync({
      "source/model.gltf": new TextEncoder().encode("{}"),
      "textures/diffuse.png": new TextEncoder().encode("fake-png-bytes"),
    });
    const zipFile = new File([zipped], "model.zip");

    const set = await buildImportFileSet(zipFile);

    expect(new Set(set.keys())).toEqual(new Set(["source/model.gltf", "textures/diffuse.png"]));
  });

  it("skips directory entries in a zip", async () => {
    const zipped = zipSync({
      "textures/": new Uint8Array(0),
      "textures/diffuse.png": new TextEncoder().encode("fake-png-bytes"),
    });
    const zipFile = new File([zipped], "model.zip");

    const set = await buildImportFileSet(zipFile);

    expect([...set.keys()]).toEqual(["textures/diffuse.png"]);
  });

  it("produces File objects whose .name is the entry's basename, for extension-based dispatch", async () => {
    const zipped = zipSync({
      "source/model.gltf": new TextEncoder().encode("{}"),
    });
    const zipFile = new File([zipped], "model.zip");

    const set = await buildImportFileSet(zipFile);

    expect(set.get("source/model.gltf")?.name).toBe("model.gltf");
  });

  it("decompresses a zip even when it arrives wrapped in a one-element array (the file-picker's actual shape, since its input has `multiple` set)", async () => {
    const zipped = zipSync({
      "model.gltf": new TextEncoder().encode("{}"),
    });
    const zipFile = new File([zipped], "model.zip");

    const set = await buildImportFileSet([zipFile]);

    expect([...set.keys()]).toEqual(["model.gltf"]);
  });
});
