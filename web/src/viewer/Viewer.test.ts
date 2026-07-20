import { describe, expect, it, vi } from "vitest";
import * as THREE from "three";
import { Viewer, type RendererLike } from "./Viewer";
import { AUTO_FIT_TARGET_SIZE } from "./AutoFit";
import { UnsupportedFormatError } from "./ImportResolver";
import { LightingPresetManager, type EnvironmentProcessor } from "./LightingPresets";
import { countMeshes, loadFixture, loadFolderFixture } from "../test/fixtures";

function createStubRenderer(canvas: HTMLCanvasElement): RendererLike {
  return {
    domElement: canvas,
    setSize: vi.fn(),
    setPixelRatio: vi.fn(),
    render: vi.fn(),
    dispose: vi.fn(),
  };
}

// PMREM prefiltering needs a live WebGL context (unavailable in jsdom), so
// it's stubbed as an identity pass-through — enough to prove *which*
// processed texture ends up bound to scene.environment and how many times
// processing actually ran, without needing a real GPU.
function createStubEnvironmentProcessor() {
  return {
    process: vi.fn((source: THREE.Texture) => source),
    dispose: vi.fn(),
  } satisfies EnvironmentProcessor;
}

// Stands in for Viewer's default fetch()-based HDR loading — resolves to
// real, tiny, committed fixture files (see scripts/generate-test-assets.mjs)
// so setLightingPreset's tests exercise the actual HDRLoader.parse()
// codepath rather than an in-memory stub texture.
function createStubLoadHdrBuffer() {
  return vi.fn(async (path: string) => {
    const fixtureName = path.includes("night") ? "fixture-night.hdr" : "fixture-day.hdr";
    return loadFixture(fixtureName, "application/octet-stream").arrayBuffer();
  });
}

function createViewer() {
  const canvas = document.createElement("canvas");
  const renderer = createStubRenderer(canvas);
  const environmentProcessor = createStubEnvironmentProcessor();
  const loadHdrBuffer = createStubLoadHdrBuffer();
  const lightingPresetManager = new LightingPresetManager(environmentProcessor, loadHdrBuffer);
  const viewer = new Viewer({ canvas, renderer, lightingPresetManager, width: 800, height: 600 });
  return { viewer, canvas, renderer, environmentProcessor, loadHdrBuffer };
}

describe("Viewer", () => {
  it("configures orbit controls with damping enabled and panning disabled", () => {
    const { viewer } = createViewer();
    expect(viewer.controls.enableDamping).toBe(true);
    expect(viewer.controls.enablePan).toBe(false);
  });

  it("auto-rotates on load", () => {
    const { viewer } = createViewer();
    expect(viewer.controls.autoRotate).toBe(true);
  });

  it("stops auto-rotate permanently after the first pointerdown on the canvas", () => {
    const { viewer, canvas } = createViewer();

    canvas.dispatchEvent(new Event("pointerdown"));
    expect(viewer.controls.autoRotate).toBe(false);

    // Re-enabling manually and firing again should not be required to stay off —
    // simulates that the stop is permanent for the session, not re-armed.
    viewer.controls.autoRotate = true;
    canvas.dispatchEvent(new Event("pointerdown"));
    expect(viewer.controls.autoRotate).toBe(true); // listener only fired once
  });

  it("restores the initial camera position and controls target on resetView()", () => {
    const { viewer } = createViewer();

    const initialPosition = viewer.camera.position.clone();
    const initialTarget = viewer.controls.target.clone();

    viewer.camera.position.set(99, 99, 99);
    viewer.controls.target.set(5, 5, 5);

    viewer.resetView();

    expect(viewer.camera.position.equals(initialPosition)).toBe(true);
    expect(viewer.controls.target.equals(initialTarget)).toBe(true);
  });

  describe("setLightingPreset", () => {
    it("processes and binds the day environment map", async () => {
      const { viewer, environmentProcessor } = createViewer();

      await viewer.setLightingPreset("day");

      expect(viewer.scene.environment).not.toBeNull();
      expect(viewer.lightingPreset).toBe("day");
      expect(environmentProcessor.process).toHaveBeenCalledTimes(1);
    });

    it("binds a different environment texture for night than for day", async () => {
      const { viewer } = createViewer();

      await viewer.setLightingPreset("day");
      const dayEnvironment = viewer.scene.environment;
      await viewer.setLightingPreset("night");
      const nightEnvironment = viewer.scene.environment;

      expect(dayEnvironment).not.toBeNull();
      expect(nightEnvironment).not.toBeNull();
      expect(dayEnvironment).not.toBe(nightEnvironment);
    });

    it("leaves scene.background untouched in the default studio Background mode", async () => {
      const { viewer } = createViewer();
      const backgroundBefore = viewer.scene.background;

      await viewer.setLightingPreset("day");
      await viewer.setLightingPreset("night");

      expect(viewer.scene.background).toBe(backgroundBefore);
    });
  });

  describe("setBackgroundMode", () => {
    it("defaults to the studio background", () => {
      const { viewer } = createViewer();

      expect(viewer.backgroundMode).toBe("studio");
      expect(viewer.scene.background).toBeInstanceOf(THREE.Color);
    });

    it("switches scene.background to the active Lighting preset's environment texture in hdri mode", async () => {
      const { viewer } = createViewer();
      await viewer.setLightingPreset("day");

      viewer.setBackgroundMode("hdri");

      expect(viewer.backgroundMode).toBe("hdri");
      expect(viewer.scene.background).toBe(viewer.scene.environment);
    });

    it("reverts scene.background to the studio color when switched back to studio", async () => {
      const { viewer } = createViewer();
      await viewer.setLightingPreset("day");
      viewer.setBackgroundMode("hdri");

      viewer.setBackgroundMode("studio");

      expect(viewer.backgroundMode).toBe("studio");
      expect(viewer.scene.background).toBeInstanceOf(THREE.Color);
    });

    it("updates the visible hdri background when the Lighting preset changes while in hdri mode", async () => {
      const { viewer } = createViewer();
      await viewer.setLightingPreset("day");
      viewer.setBackgroundMode("hdri");
      const dayBackground = viewer.scene.background;

      await viewer.setLightingPreset("night");

      expect(viewer.scene.background).not.toBe(dayBackground);
      expect(viewer.scene.background).toBe(viewer.scene.environment);
    });

    it("falls back to the studio color if hdri mode is requested before any Lighting preset has loaded", () => {
      const { viewer } = createViewer();

      viewer.setBackgroundMode("hdri");

      expect(viewer.scene.background).toBeInstanceOf(THREE.Color);
    });
  });

  it("resizes the camera aspect and renderer on resize()", () => {
    const { viewer, renderer } = createViewer();
    viewer.resize(1024, 512);
    expect(viewer.camera.aspect).toBeCloseTo(1024 / 512);
    expect(renderer.setSize).toHaveBeenCalledWith(1024, 512);
  });

  describe("importModel", () => {
    it("adds the loaded model's meshes to the scene", async () => {
      const { viewer } = createViewer();
      const file = loadFixture("fixture-simple-box.glb", "model/gltf-binary");

      await viewer.importModel(file);

      expect(countMeshes(viewer.scene)).toBeGreaterThan(0);
    });

    it("replaces the previous model rather than accumulating meshes", async () => {
      const { viewer } = createViewer();
      const first = loadFixture("fixture-simple-box.glb", "model/gltf-binary");
      const second = loadFixture("fixture-offset-box.glb", "model/gltf-binary");

      await viewer.importModel(first);
      const meshesAfterFirst = countMeshes(viewer.scene);
      await viewer.importModel(second);
      const meshesAfterSecond = countMeshes(viewer.scene);

      expect(meshesAfterSecond).toBe(meshesAfterFirst);
    });

    it("auto-fits the imported model: centered at the origin, largest dimension normalized", async () => {
      const { viewer } = createViewer();
      // 2x4x2 box originally centered at (5,5,5) — see scripts/generate-test-assets.mjs.
      const file = loadFixture("fixture-offset-box.glb", "model/gltf-binary");

      const { model } = await viewer.importModel(file);

      const box = new THREE.Box3().setFromObject(model);
      const size = box.getSize(new THREE.Vector3());
      const center = box.getCenter(new THREE.Vector3());

      expect(Math.max(size.x, size.y, size.z)).toBeCloseTo(AUTO_FIT_TARGET_SIZE, 1);
      expect(center.x).toBeCloseTo(0, 1);
      expect(center.y).toBeCloseTo(0, 1);
      expect(center.z).toBeCloseTo(0, 1);
    });

    it("rejects with UnsupportedFormatError for an unrecognized extension and leaves the scene unchanged", async () => {
      const { viewer } = createViewer();
      const meshesBefore = countMeshes(viewer.scene);
      const file = loadFixture("fixture-unsupported.txt", "text/plain");

      await expect(viewer.importModel(file)).rejects.toBeInstanceOf(UnsupportedFormatError);
      expect(countMeshes(viewer.scene)).toBe(meshesBefore);
    });

    // All 5 supported formats go through the exact same importModel/AutoFit
    // path — no per-format special-casing outside the dispatcher itself.
    it.each([
      { format: "glTF/GLB", files: ["fixture-simple-box.glb"] },
      { format: "OBJ+MTL", files: ["fixture-box.obj", "fixture-box.mtl"] },
      { format: "FBX", files: ["fixture-box-zup.fbx"] },
      { format: "STL", files: ["fixture-box.stl"] },
      { format: "Collada", files: ["fixture-box-zup.dae"] },
    ])("auto-fits a $format import the same way as every other format", async ({ files }) => {
      const { viewer } = createViewer();
      const inputFiles = files.map((name) => loadFixture(name, "application/octet-stream"));

      const { model } = await viewer.importModel(inputFiles);

      const box = new THREE.Box3().setFromObject(model);
      const size = box.getSize(new THREE.Vector3());
      const center = box.getCenter(new THREE.Vector3());

      expect(Math.max(size.x, size.y, size.z)).toBeCloseTo(AUTO_FIT_TARGET_SIZE, 1);
      expect(center.length()).toBeLessThan(0.1);
    });

    it("imports a zip file containing a flat multi-file glTF (.gltf + .bin at one level)", async () => {
      const { viewer } = createViewer();
      const zip = loadFixture("fixture-model-flat.zip", "application/zip");

      const { model, missingResources } = await viewer.importModel(zip);

      expect(countMeshes(model)).toBeGreaterThan(0);
      expect(missingResources).toEqual([]);
    });

    it("imports a zip file with the model and buffer in different subfolders, resolving the buffer via the recursive fallback", async () => {
      const { viewer } = createViewer();
      const zip = loadFixture("fixture-model-nested.zip", "application/zip");

      const { model, missingResources } = await viewer.importModel(zip);

      expect(countMeshes(model)).toBeGreaterThan(0);
      expect(missingResources).toEqual([]);
    });

    it("imports a folder selected via the file picker (flat layout)", async () => {
      const { viewer } = createViewer();
      const files = [
        loadFolderFixture("MyModel", "fixture-multifile-model.gltf", "fixture-multifile-model.gltf", "model/gltf+json"),
        loadFolderFixture("MyModel", "fixture-multifile-model.bin", "fixture-multifile-model.bin", "application/octet-stream"),
      ];

      const { model, missingResources } = await viewer.importModel(files);

      expect(countMeshes(model)).toBeGreaterThan(0);
      expect(missingResources).toEqual([]);
    });

    it("imports a folder with nested subfolders selected via the file picker", async () => {
      const { viewer } = createViewer();
      const files = [
        loadFolderFixture("MyModel", "source/fixture-multifile-model.gltf", "fixture-multifile-model.gltf", "model/gltf+json"),
        loadFolderFixture("MyModel", "buffers/fixture-multifile-model.bin", "fixture-multifile-model.bin", "application/octet-stream"),
      ];

      const { model, missingResources } = await viewer.importModel(files);

      expect(countMeshes(model)).toBeGreaterThan(0);
      expect(missingResources).toEqual([]);
    });
  });

  describe("setScale", () => {
    it("is a no-op when no model has been imported yet", () => {
      const { viewer } = createViewer();

      expect(() => viewer.setScale(2)).not.toThrow();
      expect(viewer.scale).toBe(2);
    });

    it("scales the model on top of the Auto-fit baseline rather than replacing it", async () => {
      const { viewer } = createViewer();
      const { model } = await viewer.importModel(loadFixture("fixture-simple-box.glb", "model/gltf-binary"));
      const autoFitScale = model.scale.x;

      viewer.setScale(2);

      expect(model.scale.x).toBeCloseTo(autoFitScale * 2);
      expect(model.scale.y).toBeCloseTo(autoFitScale * 2);
      expect(model.scale.z).toBeCloseTo(autoFitScale * 2);
      expect(viewer.scale).toBe(2);
    });

    it("keeps the model centered at the origin as scale changes, even when its own local origin isn't at its bounding-box center", async () => {
      const { viewer } = createViewer();
      // Deliberately off-center local origin (2x4x2 box authored at (5,5,5) —
      // see scripts/generate-test-assets.mjs) — stress-tests that position is
      // recomputed from the new scale rather than left as Auto-fit set it.
      const { model } = await viewer.importModel(loadFixture("fixture-offset-box.glb", "model/gltf-binary"));

      viewer.setScale(1.5);

      const center = new THREE.Box3().setFromObject(model).getCenter(new THREE.Vector3());
      expect(center.length()).toBeLessThan(0.1);
    });

    it("resets to 1 (the Auto-fit baseline) when a new model is imported", async () => {
      const { viewer } = createViewer();
      await viewer.importModel(loadFixture("fixture-simple-box.glb", "model/gltf-binary"));
      viewer.setScale(2);

      await viewer.importModel(loadFixture("fixture-offset-box.glb", "model/gltf-binary"));

      expect(viewer.scale).toBe(1);
    });
  });

  describe("setRotation", () => {
    it("is a no-op when no model has been imported yet", () => {
      const { viewer } = createViewer();

      expect(() => viewer.setRotation("y", 45)).not.toThrow();
      expect(viewer.rotation).toEqual({ x: 0, y: 45, z: 0 });
    });

    it("rotates on top of the Auto-orientation baseline — around the axis as currently displayed, not the raw mesh's pre-correction axis", async () => {
      const { viewer } = createViewer();
      // fixture-box-zup.fbx has a genuinely non-identity Auto-orientation
      // baseline (FBXLoader's Z-up correction) — the case where composition
      // order actually matters, since the two orders only coincide when the
      // baseline is identity.
      const { model } = await viewer.importModel(loadFixture("fixture-box-zup.fbx", "application/octet-stream"));
      const baseQuaternion = model.quaternion.clone();
      const rawPoint = new THREE.Vector3(1, 0, 0);

      viewer.setRotation("y", 90);

      // Computed independently of Viewer's own Quaternion#multiply call (via
      // applyAxisAngle instead), so this can't be tautologically satisfied by
      // whichever composition order the implementation happens to use: apply
      // Auto-orientation first, THEN rotate the result 90° around the world Y
      // axis — that's what "on top of" concretely means.
      const expected = rawPoint
        .clone()
        .applyQuaternion(baseQuaternion)
        .applyAxisAngle(new THREE.Vector3(0, 1, 0), THREE.MathUtils.degToRad(90));
      const actual = rawPoint.clone().applyQuaternion(model.quaternion);

      expect(actual.x).toBeCloseTo(expected.x);
      expect(actual.y).toBeCloseTo(expected.y);
      expect(actual.z).toBeCloseTo(expected.z);
      expect(viewer.rotation).toEqual({ x: 0, y: 90, z: 0 });
    });

    it("adjusts each axis independently without the others fighting each other", async () => {
      const { viewer } = createViewer();
      await viewer.importModel(loadFixture("fixture-simple-box.glb", "model/gltf-binary"));

      viewer.setRotation("x", 30);
      viewer.setRotation("y", 45);
      viewer.setRotation("z", -20);
      viewer.setRotation("y", 90); // overwrite y only — x/z should be unaffected

      expect(viewer.rotation).toEqual({ x: 30, y: 90, z: -20 });
    });

    it("does not change the model's scale", async () => {
      const { viewer } = createViewer();
      const { model } = await viewer.importModel(loadFixture("fixture-simple-box.glb", "model/gltf-binary"));
      const scaleBefore = model.scale.clone();

      viewer.setRotation("x", 45);

      expect(model.scale.equals(scaleBefore)).toBe(true);
    });

    it("keeps the model centered at the origin as rotation changes, even when its own local origin isn't at its bounding-box center", async () => {
      const { viewer } = createViewer();
      const { model } = await viewer.importModel(loadFixture("fixture-offset-box.glb", "model/gltf-binary"));

      viewer.setRotation("y", 45);

      const center = new THREE.Box3().setFromObject(model).getCenter(new THREE.Vector3());
      expect(center.length()).toBeLessThan(0.1);
    });

    it("resets to zero on every axis when a new model is imported", async () => {
      const { viewer } = createViewer();
      await viewer.importModel(loadFixture("fixture-simple-box.glb", "model/gltf-binary"));
      viewer.setRotation("x", 45);

      await viewer.importModel(loadFixture("fixture-offset-box.glb", "model/gltf-binary"));

      expect(viewer.rotation).toEqual({ x: 0, y: 0, z: 0 });
    });
  });
});
