import "./style.css";
import { Viewer, type LightingPreset } from "./viewer/Viewer";
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
const browseButton = requireElement<HTMLButtonElement>("#browse-button");
const fileInput = requireElement<HTMLInputElement>("#file-input");
const browseFolderButton = requireElement<HTMLButtonElement>("#browse-folder-button");
const folderInput = requireElement<HTMLInputElement>("#folder-input");
const importError = requireElement<HTMLElement>("#import-error");

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

window.addEventListener("resize", () => {
  viewer.resize(viewportRegion.clientWidth, viewportRegion.clientHeight);
});

async function importFiles(files: File[]): Promise<void> {
  importError.hidden = true;
  importError.classList.remove("warning");
  try {
    const { missingResources } = await viewer.importModel(files);
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
