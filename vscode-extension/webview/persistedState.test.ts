import { describe, expect, it, vi } from "vitest";
import { Viewer, type RendererLike } from "../../web/src/viewer/Viewer";
import { LightingPresetManager, type EnvironmentProcessor } from "../../web/src/viewer/LightingPresets";
import { capturePersistedState, restoreCameraPose, restoreScaleAndRotation, type PersistedState } from "./persistedState";

function createStubRenderer(canvas: HTMLCanvasElement): RendererLike {
  return {
    domElement: canvas,
    setSize: vi.fn(),
    setPixelRatio: vi.fn(),
    render: vi.fn(),
    dispose: vi.fn(),
  };
}

// Mirrors web/src/viewer/Viewer.test.ts's stub pattern — PMREM prefiltering
// needs a live WebGL context (unavailable in jsdom), and none of these
// tests exercise setLightingPreset, so the HDR loader is never actually
// invoked; its return value doesn't matter.
function createViewer(): Viewer {
  const canvas = document.createElement("canvas");
  const renderer = createStubRenderer(canvas);
  const environmentProcessor: EnvironmentProcessor = {
    process: vi.fn((source) => source),
    dispose: vi.fn(),
  };
  const lightingPresetManager = new LightingPresetManager(
    environmentProcessor,
    vi.fn(async () => new ArrayBuffer(0)),
  );
  return new Viewer({ canvas, renderer, lightingPresetManager, width: 800, height: 600 });
}

const baseState: PersistedState = {
  scale: 1,
  rotation: { x: 0, y: 0, z: 0 },
  lightingPreset: "day",
  backgroundMode: "studio",
  cameraPosition: { x: 0, y: 0, z: 0 },
  cameraTarget: { x: 0, y: 0, z: 0 },
};

describe("capturePersistedState", () => {
  it("snapshots scale, rotation, and camera pose from a Viewer", () => {
    const viewer = createViewer();
    viewer.setScale(2);
    viewer.setRotation("y", 45);
    viewer.camera.position.set(1, 2, 3);
    viewer.controls.target.set(4, 5, 6);

    const state = capturePersistedState(viewer);
    expect(state.scale).toBe(2);
    expect(state.rotation).toEqual({ x: 0, y: 45, z: 0 });
    expect(state.cameraPosition).toEqual({ x: 1, y: 2, z: 3 });
    expect(state.cameraTarget).toEqual({ x: 4, y: 5, z: 6 });
    expect(state.backgroundMode).toBe("studio");
  });

  it("defaults lightingPreset to 'day' before any setLightingPreset call resolves", () => {
    const viewer = createViewer();
    expect(capturePersistedState(viewer).lightingPreset).toBe("day");
  });
});

describe("restoreCameraPose", () => {
  it("sets the camera position and controls target directly, independent of any loaded model", () => {
    const viewer = createViewer();
    restoreCameraPose(viewer, { ...baseState, cameraPosition: { x: 7, y: 8, z: 9 }, cameraTarget: { x: 1, y: 1, z: 1 } });

    // toBeCloseTo, not toEqual — controls.update() round-trips the position
    // through OrbitControls' internal spherical-coordinate representation,
    // which introduces harmless floating-point noise (matches the tolerance
    // convention already used throughout Viewer.test.ts).
    const [x, y, z] = viewer.camera.position.toArray();
    expect(x).toBeCloseTo(7);
    expect(y).toBeCloseTo(8);
    expect(z).toBeCloseTo(9);
    const [tx, ty, tz] = viewer.controls.target.toArray();
    expect(tx).toBeCloseTo(1);
    expect(ty).toBeCloseTo(1);
    expect(tz).toBeCloseTo(1);
  });
});

describe("restoreScaleAndRotation", () => {
  it("applies the persisted scale/rotation on top of whatever import just reset them to", () => {
    const viewer = createViewer();
    restoreScaleAndRotation(viewer, { ...baseState, scale: 1.5, rotation: { x: 10, y: 20, z: 30 } });

    expect(viewer.scale).toBe(1.5);
    expect(viewer.rotation).toEqual({ x: 10, y: 20, z: 30 });
  });
});
