import { describe, expect, it } from "vitest";
import * as THREE from "three";
import { resolveImportedFile, UnsupportedFormatError } from "./ImportResolver";
import { countMeshes, loadFixture } from "../test/fixtures";

describe("resolveImportedFile", () => {
  it("loads a .glb file into a THREE.Object3D", async () => {
    const file = loadFixture("fixture-simple-box.glb", "model/gltf-binary");

    const object = await resolveImportedFile([file]);

    expect(object).toBeInstanceOf(THREE.Object3D);
    expect(countMeshes(object)).toBeGreaterThan(0);
  });

  it("loads a .gltf-named file the same way (GLTFLoader auto-detects GLB vs JSON)", async () => {
    // Our fixture is GLB-encoded but this exercises the .gltf extension
    // branch of the dispatcher, independent of the binary contents.
    const file = loadFixture("fixture-simple-box.glb", "model/gltf-binary");
    const renamed = new File([file], "model.gltf", { type: "model/gltf+json" });

    const object = await resolveImportedFile([renamed]);

    expect(object).toBeInstanceOf(THREE.Object3D);
  });

  it("loads a .obj file with its companion .mtl material applied", async () => {
    const obj = loadFixture("fixture-box.obj", "text/plain");
    const mtl = loadFixture("fixture-box.mtl", "text/plain");

    const object = await resolveImportedFile([obj, mtl]);

    expect(countMeshes(object)).toBeGreaterThan(0);
    let mesh: THREE.Mesh | undefined;
    object.traverse((child) => {
      if ((child as THREE.Mesh).isMesh) mesh = child as THREE.Mesh;
    });
    const material = mesh?.material as THREE.MeshPhongMaterial | undefined;
    expect(material?.name).toBe("BoxMaterial");
  });

  it("loads a .obj file without a companion .mtl (falls back to a default material)", async () => {
    const obj = loadFixture("fixture-box.obj", "text/plain");

    const object = await resolveImportedFile([obj]);

    expect(countMeshes(object)).toBeGreaterThan(0);
  });

  it("loads a .stl file, wrapped in a mesh with a default material", async () => {
    const file = loadFixture("fixture-box.stl", "model/stl");

    const object = await resolveImportedFile([file]);

    expect(countMeshes(object)).toBeGreaterThan(0);
  });

  it("loads a Z-up .fbx file and applies FBXLoader's coordinate-system correction", async () => {
    const file = loadFixture("fixture-box-zup.fbx", "application/octet-stream");

    const object = await resolveImportedFile([file]);

    // FBXLoader rotates Z-up assets by -90° around X to bring them to Y-up.
    expect(object.rotation.x).toBeCloseTo(-Math.PI / 2);
  });

  it("loads a Z-up .dae file and applies ColladaLoader's up-axis correction", async () => {
    const file = loadFixture("fixture-box-zup.dae", "model/vnd.collada+xml");

    const object = await resolveImportedFile([file]);

    expect(countMeshes(object)).toBeGreaterThan(0);
    expect(object.rotation.x).toBeCloseTo(-Math.PI / 2);
  });

  it("does not rotate .obj imports (no orientation heuristic for formats without up-axis metadata)", async () => {
    const obj = loadFixture("fixture-box.obj", "text/plain");

    const object = await resolveImportedFile([obj]);

    expect(object.rotation.x).toBe(0);
    expect(object.rotation.y).toBe(0);
    expect(object.rotation.z).toBe(0);
  });

  it("does not rotate .stl imports (no orientation heuristic for formats without up-axis metadata)", async () => {
    const file = loadFixture("fixture-box.stl", "model/stl");

    const object = await resolveImportedFile([file]);

    expect(object.rotation.x).toBe(0);
    expect(object.rotation.y).toBe(0);
    expect(object.rotation.z).toBe(0);
  });

  it("rejects with UnsupportedFormatError for an unrecognized extension", async () => {
    const file = loadFixture("fixture-unsupported.txt", "text/plain");

    await expect(resolveImportedFile([file])).rejects.toBeInstanceOf(UnsupportedFormatError);
  });

  it("includes the filename in the unsupported-format error message", async () => {
    const file = loadFixture("fixture-unsupported.txt", "text/plain");

    await expect(resolveImportedFile([file])).rejects.toThrow(/fixture-unsupported\.txt/);
  });
});
