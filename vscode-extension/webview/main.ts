import "../../web/src/style.css";
import * as THREE from "three";
import { Viewer, type BackgroundMode, type LightingPreset, type RotationAxis } from "../../web/src/viewer/Viewer";
import { LightingPresetManager, PMREMEnvironmentProcessor } from "../../web/src/viewer/LightingPresets";
import { filesFromRelayedEntries, type RelayedFileEntry } from "./filesFromRelayedEntries";
import { capturePersistedState, restoreCameraPose, restoreScaleAndRotation, type PersistedState } from "./persistedState";

declare global {
  interface Window {
    __RENDERIT_ASSET_BASE_URI__: string;
  }
}

interface VsCodeApi {
  postMessage(message: unknown): void;
  getState(): unknown;
  setState(state: unknown): void;
}
declare function acquireVsCodeApi(): VsCodeApi;

// Called exactly once per webview session, per VS Code's API contract (a
// second call throws) — see docs/research/vscode-webview-constraints.md §3.
const vscodeApi = acquireVsCodeApi();

// Set on a previous session, before this hide/show cycle destroyed and
// recreated the webview's script context — see persistedState.ts for why
// the *model* doesn't need to be part of this (the host re-relays it
// unprompted) and why camera restoration doesn't wait for scale/rotation
// restoration (issue #30).
const persistedState = vscodeApi.getState() as PersistedState | undefined;

function requireElement<T extends Element>(selector: string): T {
  const element = document.querySelector<T>(selector);
  if (!element) {
    throw new Error(`[webview] Expected page shell element not found: ${selector}`);
  }
  return element;
}

const canvas = requireElement<HTMLCanvasElement>("#viewer-canvas");
const viewportRegion = requireElement<HTMLElement>("#viewport-region");
const resetViewButton = requireElement<HTMLButtonElement>("#reset-view-button");
const lightingPresetDayButton = requireElement<HTMLButtonElement>("#lighting-preset-day-button");
const lightingPresetNightButton = requireElement<HTMLButtonElement>("#lighting-preset-night-button");
const backgroundModeStudioButton = requireElement<HTMLButtonElement>("#background-mode-studio-button");
const backgroundModeHdriButton = requireElement<HTMLButtonElement>("#background-mode-hdri-button");
const scaleSlider = requireElement<HTMLInputElement>("#scale-slider");
const scaleValue = requireElement<HTMLOutputElement>("#scale-value");
const rotationSliders: Record<RotationAxis, HTMLInputElement> = {
  x: requireElement<HTMLInputElement>("#rotation-x-slider"),
  y: requireElement<HTMLInputElement>("#rotation-y-slider"),
  z: requireElement<HTMLInputElement>("#rotation-z-slider"),
};
const rotationValues: Record<RotationAxis, HTMLOutputElement> = {
  x: requireElement<HTMLOutputElement>("#rotation-x-value"),
  y: requireElement<HTMLOutputElement>("#rotation-y-value"),
  z: requireElement<HTMLOutputElement>("#rotation-z-value"),
};
const importError = requireElement<HTMLElement>("#import-error");
const metadataFileName = requireElement<HTMLElement>("#metadata-file-name");
const metadataFormat = requireElement<HTMLElement>("#metadata-format");
const metadataFileSize = requireElement<HTMLElement>("#metadata-file-size");
const metadataTriangleCount = requireElement<HTMLElement>("#metadata-triangle-count");
const metadataVertexCount = requireElement<HTMLElement>("#metadata-vertex-count");
const metadataMeshCount = requireElement<HTMLElement>("#metadata-mesh-count");
const metadataMaterialCount = requireElement<HTMLElement>("#metadata-material-count");
const metadataTextureCount = requireElement<HTMLElement>("#metadata-texture-count");
const metadataDimensions = requireElement<HTMLElement>("#metadata-dimensions");

// Viewer's default LightingPresetManager fetches via `import.meta.env.BASE_URL`,
// which doesn't resolve inside the webview sandbox — HDRIs are instead loaded
// from the extension-host-supplied asWebviewUri base (see
// docs/research/vscode-webview-constraints.md §2, §7). Everything else about
// Viewer/LightingPresets is reused completely unmodified.
const webglRenderer = new THREE.WebGLRenderer({ canvas, antialias: true });
const lightingPresetManager = new LightingPresetManager(new PMREMEnvironmentProcessor(webglRenderer), (path) =>
  fetch(`${window.__RENDERIT_ASSET_BASE_URI__}/${path}`).then((response) => response.arrayBuffer()),
);

const viewer = new Viewer({
  canvas,
  width: viewportRegion.clientWidth,
  height: viewportRegion.clientHeight,
  renderer: webglRenderer,
  lightingPresetManager,
});

viewer.start();

// Independent of any model being loaded — see persistedState.ts.
if (persistedState) {
  restoreCameraPose(viewer, persistedState);
}

// getState/setState, not retainContextWhenHidden — VS Code's own docs
// describe the latter as having "high memory overhead" and recommend it
// only when other persistence techniques don't work (issue #30).
function persistState(): void {
  vscodeApi.setState(capturePersistedState(viewer));
}

viewer.controls.addEventListener("end", persistState);

resetViewButton.addEventListener("click", () => {
  viewer.resetView();
  persistState();
});

function setLightingPresetButtonState(preset: LightingPreset): void {
  lightingPresetDayButton.setAttribute("aria-pressed", String(preset === "day"));
  lightingPresetNightButton.setAttribute("aria-pressed", String(preset === "night"));
}

async function applyLightingPreset(preset: LightingPreset): Promise<void> {
  await viewer.setLightingPreset(preset);
  setLightingPresetButtonState(preset);
}

lightingPresetDayButton.addEventListener("click", () => void applyLightingPreset("day").then(persistState));
lightingPresetNightButton.addEventListener("click", () => void applyLightingPreset("night").then(persistState));

function setBackgroundModeButtonState(mode: BackgroundMode): void {
  backgroundModeStudioButton.setAttribute("aria-pressed", String(mode === "studio"));
  backgroundModeHdriButton.setAttribute("aria-pressed", String(mode === "hdri"));
}

function applyBackgroundMode(mode: BackgroundMode): void {
  viewer.setBackgroundMode(mode);
  setBackgroundModeButtonState(mode);
}

backgroundModeStudioButton.addEventListener("click", () => {
  applyBackgroundMode("studio");
  persistState();
});
backgroundModeHdriButton.addEventListener("click", () => {
  applyBackgroundMode("hdri");
  persistState();
});

scaleSlider.addEventListener("input", () => {
  const value = Number(scaleSlider.value);
  viewer.setScale(value);
  scaleValue.textContent = value.toFixed(2);
  persistState();
});

for (const axis of Object.keys(rotationSliders) as RotationAxis[]) {
  rotationSliders[axis].addEventListener("input", () => {
    const degrees = Number(rotationSliders[axis].value);
    viewer.setRotation(axis, degrees);
    rotationValues[axis].textContent = `${degrees}°`;
    persistState();
  });
}

function syncScaleRotationControls(): void {
  scaleSlider.value = String(viewer.scale);
  scaleValue.textContent = viewer.scale.toFixed(2);
  for (const axis of Object.keys(rotationSliders) as RotationAxis[]) {
    const degrees = viewer.rotation[axis];
    rotationSliders[axis].value = String(degrees);
    rotationValues[axis].textContent = `${degrees}°`;
  }
}

window.addEventListener("resize", () => {
  viewer.resize(viewportRegion.clientWidth, viewportRegion.clientHeight);
});

function formatFileSize(bytes: number): string {
  if (bytes < 1024) return `${bytes} B`;
  const units = ["KB", "MB", "GB"];
  let value = bytes / 1024;
  let unitIndex = 0;
  while (value >= 1024 && unitIndex < units.length - 1) {
    value /= 1024;
    unitIndex++;
  }
  return `${value.toFixed(1)} ${units[unitIndex]}`;
}

function renderMetadataPanel(): void {
  const metadata = viewer.metadata;
  if (!metadata) return;

  metadataFileName.textContent = metadata.fileName;
  metadataFormat.textContent = metadata.format;
  metadataFileSize.textContent = formatFileSize(metadata.fileSizeBytes);
  metadataTriangleCount.textContent = metadata.triangleCount.toLocaleString();
  metadataVertexCount.textContent = metadata.vertexCount.toLocaleString();
  metadataMeshCount.textContent = metadata.meshCount.toLocaleString();
  metadataMaterialCount.textContent = metadata.materialCount.toLocaleString();
  metadataTextureCount.textContent = metadata.textureCount.toLocaleString();
  const { width, height, depth } = metadata.boundingBoxSize;
  metadataDimensions.textContent = `${width.toFixed(2)} × ${height.toFixed(2)} × ${depth.toFixed(2)}`;
}

async function importFiles(source: File | File[]): Promise<void> {
  importError.hidden = true;
  importError.classList.remove("warning");
  try {
    const { missingResources } = await viewer.importModel(source);
    // A fresh import always resets scale/rotation to their 1x/0° baseline
    // (see Viewer.importModel's own comment) — restore the persisted
    // adjustment on top of that reset, not before it. Background mode is
    // restored separately, as soon as lighting resolves rather than waiting
    // on import (see below) — gating it here would flash the studio default
    // for however long import/parsing takes, since setLightingPreset()
    // applies whatever backgroundMode is currently active (still the
    // "studio" default at that point) internally, before import even starts.
    if (persistedState) {
      restoreScaleAndRotation(viewer, persistedState);
    }
    syncScaleRotationControls();
    renderMetadataPanel();
    if (missingResources.length > 0) {
      importError.textContent = `Model loaded, but couldn't find: ${missingResources.join(", ")}`;
      importError.classList.add("warning");
      importError.hidden = false;
    }
  } catch (error) {
    importError.textContent = error instanceof Error ? error.message : "Failed to import model.";
    importError.hidden = false;
  }
}

// Day is the default Lighting preset (or the persisted one, if this is a
// rehydration after the webview was hidden and shown again — issue #30) —
// started immediately (independent of the opened file arriving) but the
// model is only added to the scene once lighting has resolved, mirroring
// web/src/main.ts's own ordering: HDRI image-based lighting replaced the
// always-on fallback lights (issue #14), so without this ordering the model
// would render unlit for a visible window.
//
// Background mode restoration is chained here, not inside importFiles —
// setLightingPreset() applies whatever backgroundMode is currently active
// internally as soon as it resolves (still "studio", the Viewer default,
// until this runs), independent of whether/how long import subsequently
// takes. Restoring it later (gated on import) would flash the wrong
// background for that whole window — exactly the "visible reload flash"
// issue #30's acceptance criteria rules out.
const lightingPresetReady = applyLightingPreset(persistedState?.lightingPreset ?? "day").then(() => {
  if (persistedState) {
    applyBackgroundMode(persistedState.backgroundMode);
  }
});

interface OpenFileMessage {
  type: "openFile";
  fileName: string;
  // Never SharedArrayBuffer-backed in practice — VS Code's postMessage
  // bridge can't transfer one at all (see
  // docs/research/vscode-webview-constraints.md §4) — pinned to plain
  // ArrayBuffer so this satisfies BlobPart below without a cast.
  bytes: Uint8Array<ArrayBuffer>;
}

/** Folder (and, via ImportFileSet's own zip detection, zip-relayed-as-one-file
 * only reuses OpenFileMessage) imports arrive as multiple relayed entries —
 * see src/openWithRenderit.ts (issue #29). */
interface OpenFilesMessage {
  type: "openFiles";
  files: RelayedFileEntry[];
}

window.addEventListener("message", (event: MessageEvent<OpenFileMessage | OpenFilesMessage>) => {
  const message = event.data;
  if (message?.type === "openFile") {
    const file = new File([message.bytes], message.fileName);
    void lightingPresetReady.then(() => importFiles(file));
  } else if (message?.type === "openFiles") {
    const files = filesFromRelayedEntries(message.files);
    void lightingPresetReady.then(() => importFiles(files));
  }
});

// Signals the host that the message listener above is attached — the host
// waits for this before posting the file's bytes, since postMessage delivery
// to a not-yet-listening webview is best-effort, not guaranteed.
vscodeApi.postMessage({ type: "ready" });
