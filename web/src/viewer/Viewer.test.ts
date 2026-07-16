import { describe, expect, it, vi } from "vitest";
import * as THREE from "three";
import { Viewer, type RendererLike } from "./Viewer";
import { AUTO_FIT_TARGET_SIZE } from "./AutoFit";
import { UnsupportedFormatError } from "./ImportResolver";
import { countMeshes, loadFixture } from "../test/fixtures";

function createStubRenderer(canvas: HTMLCanvasElement): RendererLike {
  return {
    domElement: canvas,
    setSize: vi.fn(),
    setPixelRatio: vi.fn(),
    render: vi.fn(),
    dispose: vi.fn(),
  };
}

function createViewer() {
  const canvas = document.createElement("canvas");
  const renderer = createStubRenderer(canvas);
  const viewer = new Viewer({ canvas, renderer, width: 800, height: 600 });
  return { viewer, canvas, renderer };
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

  it("provides default lighting so imported models are visible", () => {
    const { viewer } = createViewer();
    let lightCount = 0;
    viewer.scene.traverse((obj) => {
      if ((obj as THREE.Light).isLight) lightCount++;
    });
    expect(lightCount).toBeGreaterThan(0);
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

      const model = await viewer.importModel(file);

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
  });
});
