import { describe, expect, it, vi } from "vitest";
import * as THREE from "three";
import { Viewer, type RendererLike } from "./Viewer";

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

  it("places a placeholder-lit object in the scene", () => {
    const { viewer } = createViewer();
    const meshes: THREE.Object3D[] = [];
    viewer.scene.traverse((obj) => {
      if ((obj as THREE.Mesh).isMesh) meshes.push(obj);
    });
    expect(meshes.length).toBeGreaterThan(0);
  });

  it("resizes the camera aspect and renderer on resize()", () => {
    const { viewer, renderer } = createViewer();
    viewer.resize(1024, 512);
    expect(viewer.camera.aspect).toBeCloseTo(1024 / 512);
    expect(renderer.setSize).toHaveBeenCalledWith(1024, 512);
  });
});
