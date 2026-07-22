import type { Viewer, BackgroundMode, LightingPreset, RotationDegrees } from "../../web/src/viewer/Viewer";

export interface PersistedState {
  scale: number;
  rotation: RotationDegrees;
  lightingPreset: LightingPreset;
  backgroundMode: BackgroundMode;
  cameraPosition: { x: number; y: number; z: number };
  cameraTarget: { x: number; y: number; z: number };
}

type PersistableViewer = Pick<Viewer, "scale" | "rotation" | "lightingPreset" | "backgroundMode" | "camera" | "controls">;

/**
 * Snapshots everything getState/setState needs to survive a webview
 * hide/show cycle (issue #30) — deliberately *not* the loaded model itself.
 * The extension host already re-relays that unprompted: a hidden webview's
 * script context is destroyed and recreated from scratch on VS Code's own
 * default behavior, but RenderitEditorProvider.ts/openWithRenderit.ts's
 * onRenderitWebviewReady handler is registered once in resolveCustomEditor
 * and stays alive across the panel's visibility changes (it's part of the
 * extension host process, not the webview's), so the freshly-booted
 * webview's new "ready" message gets the file's bytes posted to it again
 * automatically, same as the very first time.
 */
export function capturePersistedState(viewer: PersistableViewer): PersistedState {
  return {
    scale: viewer.scale,
    rotation: viewer.rotation,
    lightingPreset: viewer.lightingPreset ?? "day",
    backgroundMode: viewer.backgroundMode,
    cameraPosition: { x: viewer.camera.position.x, y: viewer.camera.position.y, z: viewer.camera.position.z },
    cameraTarget: { x: viewer.controls.target.x, y: viewer.controls.target.y, z: viewer.controls.target.z },
  };
}

/** Restores the camera pose immediately at bootstrap, independent of any
 * model being loaded — importModel() never touches camera position/target
 * (only AutoFit's scale and the model's own transform), so this doesn't
 * need to wait for anything else. Delegates to Viewer.setCameraPose() rather
 * than duplicating its auto-rotate-suppression dance (idle auto-rotate is
 * still active this early — it only stops on the canvas's first
 * pointerdown — and a naive position/target assignment would get nudged
 * off-target by the next controls.update() tick). */
export function restoreCameraPose(viewer: Pick<Viewer, "setCameraPose">, state: PersistedState): void {
  viewer.setCameraPose(state.cameraPosition, state.cameraTarget);
}

/** Must run *after* importModel() resolves — a fresh import always resets
 * scale to 1 and rotation to 0° as its new manual-adjustment baseline (see
 * Viewer.importModel's own comment), so restoring the persisted values has
 * to happen after that reset, not before it. */
export function restoreScaleAndRotation(viewer: Pick<Viewer, "setScale" | "setRotation">, state: PersistedState): void {
  viewer.setScale(state.scale);
  for (const axis of Object.keys(state.rotation) as (keyof RotationDegrees)[]) {
    viewer.setRotation(axis, state.rotation[axis]);
  }
}
