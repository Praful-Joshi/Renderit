import { describe, expect, it } from "vitest";
import * as THREE from "three";
import { resolveImportedFile, UnsupportedFormatError } from "./ImportResolver";
import { countMeshes, loadFixtureSet } from "../test/fixtures";

describe("resolveImportedFile", () => {
  it("loads a .glb file into a THREE.Object3D", async () => {
    const files = loadFixtureSet(["fixture-simple-box.glb"]);

    const { object } = await resolveImportedFile(files);

    expect(object).toBeInstanceOf(THREE.Object3D);
    expect(countMeshes(object)).toBeGreaterThan(0);
  });

  it("loads a .gltf-named file the same way (GLTFLoader auto-detects GLB vs JSON)", async () => {
    // Our fixture is GLB-encoded but this exercises the .gltf extension
    // branch of the dispatcher, independent of the binary contents.
    const glb = loadFixtureSet(["fixture-simple-box.glb"]).get("fixture-simple-box.glb")!;
    const files = new Map([["model.gltf", new File([glb], "model.gltf", { type: "model/gltf+json" })]]);

    const { object } = await resolveImportedFile(files);

    expect(object).toBeInstanceOf(THREE.Object3D);
  });

  it("loads a .obj file with its companion .mtl material applied", async () => {
    const files = loadFixtureSet(["fixture-box.obj", "fixture-box.mtl"]);

    const { object } = await resolveImportedFile(files);

    expect(countMeshes(object)).toBeGreaterThan(0);
    let mesh: THREE.Mesh | undefined;
    object.traverse((child) => {
      if ((child as THREE.Mesh).isMesh) mesh = child as THREE.Mesh;
    });
    const material = mesh?.material as THREE.MeshPhongMaterial | undefined;
    expect(material?.name).toBe("BoxMaterial");
  });

  it("loads a .obj file without a companion .mtl (falls back to a default material)", async () => {
    const files = loadFixtureSet(["fixture-box.obj"]);

    const { object } = await resolveImportedFile(files);

    expect(countMeshes(object)).toBeGreaterThan(0);
  });

  it("loads a .stl file, wrapped in a mesh with a default material", async () => {
    const files = loadFixtureSet(["fixture-box.stl"]);

    const { object } = await resolveImportedFile(files);

    expect(countMeshes(object)).toBeGreaterThan(0);
  });

  it("loads a Z-up .fbx file and applies FBXLoader's coordinate-system correction", async () => {
    const files = loadFixtureSet(["fixture-box-zup.fbx"]);

    const { object } = await resolveImportedFile(files);

    // FBXLoader rotates Z-up assets by -90° around X to bring them to Y-up.
    expect(object.rotation.x).toBeCloseTo(-Math.PI / 2);
  });

  it("loads a Z-up .dae file and applies ColladaLoader's up-axis correction", async () => {
    const files = loadFixtureSet(["fixture-box-zup.dae"]);

    const { object } = await resolveImportedFile(files);

    expect(countMeshes(object)).toBeGreaterThan(0);
    expect(object.rotation.x).toBeCloseTo(-Math.PI / 2);
  });

  it("does not rotate .obj imports (no orientation heuristic for formats without up-axis metadata)", async () => {
    const files = loadFixtureSet(["fixture-box.obj"]);

    const { object } = await resolveImportedFile(files);

    expect(object.rotation.x).toBe(0);
    expect(object.rotation.y).toBe(0);
    expect(object.rotation.z).toBe(0);
  });

  it("does not rotate .stl imports (no orientation heuristic for formats without up-axis metadata)", async () => {
    const files = loadFixtureSet(["fixture-box.stl"]);

    const { object } = await resolveImportedFile(files);

    expect(object.rotation.x).toBe(0);
    expect(object.rotation.y).toBe(0);
    expect(object.rotation.z).toBe(0);
  });

  it.each([
    { fixture: "fixture-simple-box.glb", expectedFormat: "glTF/GLB" },
    { fixture: "fixture-box.stl", expectedFormat: "STL" },
    { fixture: "fixture-box-zup.fbx", expectedFormat: "FBX" },
    { fixture: "fixture-box-zup.dae", expectedFormat: "Collada" },
  ])("reports the primary file's name and a human-readable format label for $fixture", async ({ fixture, expectedFormat }) => {
    const files = loadFixtureSet([fixture]);

    const { fileName, format } = await resolveImportedFile(files);

    expect(fileName).toBe(fixture);
    expect(format).toBe(expectedFormat);
  });

  it("reports the primary model file's name, not a companion .mtl's, for an OBJ+MTL import", async () => {
    const files = loadFixtureSet(["fixture-box.obj", "fixture-box.mtl"]);

    const { fileName, format } = await resolveImportedFile(files);

    expect(fileName).toBe("fixture-box.obj");
    expect(format).toBe("OBJ");
  });

  it("rejects with UnsupportedFormatError for an unrecognized extension", async () => {
    const files = loadFixtureSet(["fixture-unsupported.txt"]);

    await expect(resolveImportedFile(files)).rejects.toBeInstanceOf(UnsupportedFormatError);
  });

  it("includes the filename in the unsupported-format error message", async () => {
    const files = loadFixtureSet(["fixture-unsupported.txt"]);

    await expect(resolveImportedFile(files)).rejects.toThrow(/fixture-unsupported\.txt/);
  });
});
