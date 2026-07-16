import "./style.css";
import { Viewer } from "./viewer/Viewer";

const canvas = document.querySelector<HTMLCanvasElement>("#viewer-canvas");
const viewportRegion = document.querySelector<HTMLElement>("#viewport-region");
const resetViewButton = document.querySelector<HTMLButtonElement>("#reset-view-button");

if (!canvas || !viewportRegion || !resetViewButton) {
  throw new Error("[main] Expected page shell elements were not found in the DOM");
}

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
