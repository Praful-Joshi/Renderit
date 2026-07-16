import { describe, expect, it } from "vitest";
import * as THREE from "three";
import { resolveImportedFile, UnsupportedFormatError } from "./ImportResolver";
import { countMeshes, loadFixture } from "../test/fixtures";

describe("resolveImportedFile", () => {
  it("loads a .glb file into a THREE.Object3D", async () => {
    const file = loadFixture("fixture-simple-box.glb", "model/gltf-binary");

    const object = await resolveImportedFile(file);

    expect(object).toBeInstanceOf(THREE.Object3D);
    expect(countMeshes(object)).toBeGreaterThan(0);
  });

  it("loads a .gltf-named file the same way (GLTFLoader auto-detects GLB vs JSON)", async () => {
    // Our fixture is GLB-encoded but this exercises the .gltf extension
    // branch of the dispatcher, independent of the binary contents.
    const file = loadFixture("fixture-simple-box.glb", "model/gltf-binary");
    const renamed = new File([file], "model.gltf", { type: "model/gltf+json" });

    const object = await resolveImportedFile(renamed);

    expect(object).toBeInstanceOf(THREE.Object3D);
  });

  it("rejects with UnsupportedFormatError for an unrecognized extension", async () => {
    const file = loadFixture("fixture-unsupported.txt", "text/plain");

    await expect(resolveImportedFile(file)).rejects.toBeInstanceOf(UnsupportedFormatError);
  });

  it("includes the filename in the unsupported-format error message", async () => {
    const file = loadFixture("fixture-unsupported.txt", "text/plain");

    await expect(resolveImportedFile(file)).rejects.toThrow(/fixture-unsupported\.txt/);
  });
});
