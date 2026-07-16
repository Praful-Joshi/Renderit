// Regenerates the bundled showcase model and the GLB test fixtures used by
// Viewer.test.ts / ImportResolver.test.ts. Run from web/: node scripts/generate-test-assets.mjs
//
// GLTFExporter needs browser globals (document, Blob, FileReader) that plain
// Node doesn't provide — jsdom polyfills them, matching how the tests
// themselves run.
import { JSDOM } from "jsdom";

const dom = new JSDOM("<!doctype html><html><body></body></html>");
globalThis.window = dom.window;
globalThis.document = dom.window.document;
globalThis.Blob = dom.window.Blob;
globalThis.FileReader = dom.window.FileReader;

const THREE = await import("three");
const { GLTFExporter } = await import("three/addons/exporters/GLTFExporter.js");
const fs = await import("node:fs");
const path = await import("node:path");

const webRoot = path.resolve(import.meta.dirname, "..");

async function exportGlb(scene, outPath) {
  const exporter = new GLTFExporter();
  const glb = await new Promise((resolve, reject) => {
    exporter.parse(scene, resolve, reject, { binary: true });
  });
  fs.mkdirSync(path.dirname(outPath), { recursive: true });
  fs.writeFileSync(outPath, Buffer.from(glb));
  console.log("wrote", path.relative(webRoot, outPath), glb.byteLength, "bytes");
}

// Showcase model — pre-loaded into the Viewer on page load (see main.ts's
// importShowcaseModel). A stylized gem, distinct from any test fixture.
{
  const scene = new THREE.Scene();
  const mesh = new THREE.Mesh(
    new THREE.IcosahedronGeometry(1, 2),
    new THREE.MeshStandardMaterial({
      color: 0x4fd1c5,
      roughness: 0.25,
      metalness: 0.6,
      flatShading: true,
    }),
  );
  mesh.name = "ShowcaseGem";
  scene.add(mesh);
  await exportGlb(scene, path.join(webRoot, "public/models/showcase.glb"));
}

// fixture-offset-box.glb — a 2x4x2 box positioned off-center at (5,5,5), so
// AutoFit's expected scale/position can be hand-computed exactly in tests:
//   maxDimension = 4 -> scale = AUTO_FIT_TARGET_SIZE(2) / 4 = 0.5
//   center = (5,5,5) -> position = -scale * center = (-2.5,-2.5,-2.5)
{
  const scene = new THREE.Scene();
  const mesh = new THREE.Mesh(
    new THREE.BoxGeometry(2, 4, 2),
    new THREE.MeshStandardMaterial({ color: 0xff0000 }),
  );
  mesh.position.set(5, 5, 5);
  mesh.name = "OffsetBox";
  scene.add(mesh);
  await exportGlb(scene, path.join(webRoot, "src/test/fixtures/fixture-offset-box.glb"));
}

// fixture-simple-box.glb — a small, roughly-centered box for general
// import-success tests where exact fit math isn't the point.
{
  const scene = new THREE.Scene();
  const mesh = new THREE.Mesh(
    new THREE.BoxGeometry(1, 1, 1),
    new THREE.MeshStandardMaterial({ color: 0x00ff00 }),
  );
  mesh.name = "SimpleBox";
  scene.add(mesh);
  await exportGlb(scene, path.join(webRoot, "src/test/fixtures/fixture-simple-box.glb"));
}
