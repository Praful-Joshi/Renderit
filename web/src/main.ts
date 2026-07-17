import "./style.css";
import { Viewer } from "./viewer/Viewer";

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
const browseButton = requireElement<HTMLButtonElement>("#browse-button");
const fileInput = requireElement<HTMLInputElement>("#file-input");
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

window.addEventListener("resize", () => {
  viewer.resize(viewportRegion.clientWidth, viewportRegion.clientHeight);
});

async function importFiles(files: File[]): Promise<void> {
  importError.hidden = true;
  try {
    await viewer.importModel(files);
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
  const files = event.dataTransfer ? Array.from(event.dataTransfer.files) : [];
  if (files.length > 0) void importFiles(files);
});

async function importShowcaseModel(): Promise<void> {
  const url = `${import.meta.env.BASE_URL}models/showcase.glb`;
  const response = await fetch(url);
  const blob = await response.blob();
  await importFiles([new File([blob], "showcase.glb", { type: "model/gltf-binary" })]);
}

void importShowcaseModel();
