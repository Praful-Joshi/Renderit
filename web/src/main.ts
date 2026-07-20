import "./style.css";
import { Viewer, type BackgroundMode, type LightingPreset, type RotationAxis } from "./viewer/Viewer";
import { filesFromDataTransfer } from "./viewer/DragDropFiles";

function requireElement<T extends Element>(selector: string): T {
  const element = document.querySelector<T>(selector);
  if (!element) {
    throw new Error(`[main] Expected page shell element not found: ${selector}`);
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
const browseButton = requireElement<HTMLButtonElement>("#browse-button");
const fileInput = requireElement<HTMLInputElement>("#file-input");
const browseFolderButton = requireElement<HTMLButtonElement>("#browse-folder-button");
const folderInput = requireElement<HTMLInputElement>("#folder-input");
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

const viewer = new Viewer({
  canvas,
  width: viewportRegion.clientWidth,
  height: viewportRegion.clientHeight,
});

viewer.start();

resetViewButton.addEventListener("click", () => {
  viewer.resetView();
});

function setLightingPresetButtonState(preset: LightingPreset): void {
  lightingPresetDayButton.setAttribute("aria-pressed", String(preset === "day"));
  lightingPresetNightButton.setAttribute("aria-pressed", String(preset === "night"));
}

async function applyLightingPreset(preset: LightingPreset): Promise<void> {
  await viewer.setLightingPreset(preset);
  setLightingPresetButtonState(preset);
}

lightingPresetDayButton.addEventListener("click", () => void applyLightingPreset("day"));
lightingPresetNightButton.addEventListener("click", () => void applyLightingPreset("night"));

function setBackgroundModeButtonState(mode: BackgroundMode): void {
  backgroundModeStudioButton.setAttribute("aria-pressed", String(mode === "studio"));
  backgroundModeHdriButton.setAttribute("aria-pressed", String(mode === "hdri"));
}

function applyBackgroundMode(mode: BackgroundMode): void {
  viewer.setBackgroundMode(mode);
  setBackgroundModeButtonState(mode);
}

backgroundModeStudioButton.addEventListener("click", () => applyBackgroundMode("studio"));
backgroundModeHdriButton.addEventListener("click", () => applyBackgroundMode("hdri"));

// Direct call-through on input, no intermediate state layer — per issue #9's
// SettingsPanel decision and docs/adr/0004-web-ui-stack.md's "no framework,
// direct control over the scene" rationale.
scaleSlider.addEventListener("input", () => {
  const value = Number(scaleSlider.value);
  viewer.setScale(value);
  scaleValue.textContent = value.toFixed(2);
});

for (const axis of Object.keys(rotationSliders) as RotationAxis[]) {
  rotationSliders[axis].addEventListener("input", () => {
    const degrees = Number(rotationSliders[axis].value);
    viewer.setRotation(axis, degrees);
    rotationValues[axis].textContent = `${degrees}°`;
  });
}

// Viewer resets scale/rotation to their defaults on every import (a fresh
// model's manual adjustments shouldn't carry over from the last one) — sync
// the sliders/readouts to match after each successful import.
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

// The metadata panel just renders MetadataExtractor's output — per issue #9's
// SettingsPanel/MetadataPanel decision, no state layer of its own.
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

async function importFiles(files: File[]): Promise<void> {
  importError.hidden = true;
  importError.classList.remove("warning");
  try {
    const { missingResources } = await viewer.importModel(files);
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

browseButton.addEventListener("click", () => fileInput.click());

fileInput.addEventListener("change", () => {
  const files = fileInput.files ? Array.from(fileInput.files) : [];
  fileInput.value = ""; // allow re-selecting the same file(s)
  if (files.length > 0) void importFiles(files);
});

browseFolderButton.addEventListener("click", () => folderInput.click());

folderInput.addEventListener("change", () => {
  // <input webkitdirectory> sets .webkitRelativePath on every File — the
  // same shape buildImportFileSet already expects.
  const files = folderInput.files ? Array.from(folderInput.files) : [];
  folderInput.value = "";
  if (files.length > 0) void importFiles(files);
});

viewportRegion.addEventListener("dragover", (event) => {
  event.preventDefault();
  viewportRegion.classList.add("drag-active");
});

viewportRegion.addEventListener("dragleave", (event) => {
  const related = event.relatedTarget as Node | null;
  if (!related || !viewportRegion.contains(related)) {
    viewportRegion.classList.remove("drag-active");
  }
});

viewportRegion.addEventListener("drop", (event) => {
  event.preventDefault();
  viewportRegion.classList.remove("drag-active");
  if (!event.dataTransfer) return;
  void filesFromDataTransfer(event.dataTransfer).then((files) => {
    if (files.length > 0) void importFiles(files);
  });
});

async function fetchShowcaseModelFile(): Promise<File> {
  const url = `${import.meta.env.BASE_URL}models/showcase.glb`;
  const response = await fetch(url);
  const blob = await response.blob();
  return new File([blob], "showcase.glb", { type: "model/gltf-binary" });
}

// Day is the default Lighting preset on load. It's fetched/processed in
// parallel with the showcase model, but the model is only added to the
// scene (via importFiles) once lighting has resolved — HDRI image-based
// lighting replaced the old always-on fallback lights (see issue #14), so
// without this ordering the showcase model would render fully unlit/black
// for a visible window right after import.
void Promise.all([applyLightingPreset("day"), fetchShowcaseModelFile()]).then(([, showcaseFile]) =>
  importFiles([showcaseFile]),
);
