// Regenerates the bundled showcase model and the fixture files used by the
// viewer tests. Run from web/: node scripts/generate-test-assets.mjs
//
// GLTFExporter/OBJExporter/STLExporter need browser globals (document, Blob,
// FileReader) that plain Node doesn't provide — jsdom polyfills them,
// matching how the tests themselves run. The FBX and Collada fixtures need
// `assimp` (already a native-renderer dependency, brew install assimp) since
// three.js ships no exporter for either format.
import { JSDOM } from "jsdom";

const dom = new JSDOM("<!doctype html><html><body></body></html>");
globalThis.window = dom.window;
globalThis.document = dom.window.document;
globalThis.Blob = dom.window.Blob;
globalThis.FileReader = dom.window.FileReader;

const THREE = await import("three");
const { GLTFExporter } = await import("three/addons/exporters/GLTFExporter.js");
const { OBJExporter } = await import("three/addons/exporters/OBJExporter.js");
const { STLExporter } = await import("three/addons/exporters/STLExporter.js");
const fs = await import("node:fs");
const path = await import("node:path");
const os = await import("node:os");
const { execFileSync } = await import("node:child_process");
const { zipSync } = await import("fflate");

const webRoot = path.resolve(import.meta.dirname, "..");
const fixturesDir = path.join(webRoot, "src/test/fixtures");

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

// fixture-box.obj + fixture-box.mtl — a unit box with a companion material,
// for the OBJ+MTL multi-file import path (#12). OBJExporter can't emit MTL
// files itself (geometry only), so the material file is hand-written to
// match the `usemtl` name OBJExporter embeds.
{
  const material = new THREE.MeshStandardMaterial({ color: 0x3388ff });
  material.name = "BoxMaterial";
  const mesh = new THREE.Mesh(new THREE.BoxGeometry(1, 1, 1), material);

  const objText = "mtllib fixture-box.mtl\n" + new OBJExporter().parse(mesh);
  fs.writeFileSync(path.join(fixturesDir, "fixture-box.obj"), objText);
  console.log("wrote src/test/fixtures/fixture-box.obj");

  const mtlText = "newmtl BoxMaterial\nKd 0.2 0.53 1.0\n";
  fs.writeFileSync(path.join(fixturesDir, "fixture-box.mtl"), mtlText);
  console.log("wrote src/test/fixtures/fixture-box.mtl");
}

// fixture-box.stl — a unit box, binary STL.
{
  const mesh = new THREE.Mesh(new THREE.BoxGeometry(1, 1, 1), new THREE.MeshStandardMaterial());
  const stlArrayBuffer = new STLExporter().parse(mesh, { binary: true });
  fs.writeFileSync(
    path.join(fixturesDir, "fixture-box.stl"),
    Buffer.from(stlArrayBuffer.buffer, stlArrayBuffer.byteOffset, stlArrayBuffer.byteLength),
  );
  console.log("wrote src/test/fixtures/fixture-box.stl");
}

// fixture-box-zup.fbx / fixture-box-zup.dae — a unit box exported via
// assimp (three.js ships no FBX/Collada exporter), then patched to declare
// a Z-up coordinate system. Both FBXLoader and ColladaLoader read this flag
// and auto-rotate Z-up assets to Y-up (see docs/adr/0003) — assimp's default
// export is Y-up, so the patch is what actually exercises that correction
// path; the vertex data itself is untouched, same as real-world Z-up files.
//
// The intermediate OBJ assimp reads from is generated via OBJExporter (not
// hand-written) so it carries real per-face normals.
//
// Known cosmetic quirk: the .fbx fixture's material renders solid black —
// FBXLoader's ASCII-FBX property parser produces a NaN green channel from
// assimp's `P: "DiffuseColor", "Color", "", "A", …` property line (isolated
// via debug script; reproduces even with the "Diffuse" property removed, so
// it's specific to how three.js parses assimp's "A"-annotated Color
// properties, not a normals/geometry/orientation issue — rotation.x, mesh
// count, and vertex count are all correct and covered by tests). Not fixed
// here: it's a narrow assimp/FBXLoader interop edge case, not a defect in
// this project's import code.
{
  const tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), "renderit-fixtures-"));
  const tmpObj = path.join(tmpDir, "box.obj");
  const tmpMesh = new THREE.Mesh(new THREE.BoxGeometry(1, 1, 1), new THREE.MeshStandardMaterial());
  fs.writeFileSync(tmpObj, new OBJExporter().parse(tmpMesh));

  const tmpFbx = path.join(tmpDir, "box.fbx");
  execFileSync("assimp", ["export", tmpObj, tmpFbx, "-ffbxa"]);
  const fbxText = fs
    .readFileSync(tmpFbx, "utf-8")
    .replace('P: "UpAxis", "int", "Integer", "", 1', 'P: "UpAxis", "int", "Integer", "", 2');
  fs.writeFileSync(path.join(fixturesDir, "fixture-box-zup.fbx"), fbxText);
  console.log("wrote src/test/fixtures/fixture-box-zup.fbx");

  const tmpDae = path.join(tmpDir, "box.dae");
  execFileSync("assimp", ["export", tmpObj, tmpDae, "-fcollada"]);
  const daeText = fs.readFileSync(tmpDae, "utf-8").replace("<up_axis>Y_UP</up_axis>", "<up_axis>Z_UP</up_axis>");
  fs.writeFileSync(path.join(fixturesDir, "fixture-box-zup.dae"), daeText);
  console.log("wrote src/test/fixtures/fixture-box-zup.dae");

  fs.rmSync(tmpDir, { recursive: true });
}

// fixture-multifile-model.gltf + .bin — a non-binary glTF export (external
// buffer, no texture) for issue #13's zip/folder resolution tests. No image
// reference is used deliberately: jsdom's <img> never fires onload/onerror
// for any src (blob: or plain relative), regardless of whether the
// reference resolves — so any fixture with a texture hangs forever inside
// GLTFLoader.parse() under vitest. This fixture exercises the exact same
// resolution mechanism (LoadingManager.setURLModifier, relative-path-first
// + recursive-basename fallback) against the external .bin buffer instead,
// which resolves via fetch() (polyfilled in test/setup.ts), not <img>.
// Actual texture resolution is verified manually in a real browser instead
// — see the ticket's manual QA notes.
const multifileGltfName = "fixture-multifile-model.gltf";
const multifileBinName = "fixture-multifile-model.bin";
{
  const scene = new THREE.Scene();
  const mesh = new THREE.Mesh(new THREE.BoxGeometry(1, 1, 1), new THREE.MeshStandardMaterial({ color: 0x8855ff }));
  mesh.name = "MultifileBox";
  scene.add(mesh);

  const result = await new Promise((resolve, reject) => {
    new GLTFExporter().parse(scene, resolve, reject, { binary: false, embedImages: false });
  });

  // GLTFExporter embeds the buffer as a data: URI even with binary:false;
  // extract its bytes and rewrite the JSON to reference an external file
  // instead, matching what a real multi-file glTF export looks like.
  const dataUri = result.buffers[0].uri;
  const binBytes = Buffer.from(dataUri.slice(dataUri.indexOf(",") + 1), "base64");
  result.buffers[0].uri = multifileBinName;

  fs.writeFileSync(path.join(fixturesDir, multifileGltfName), JSON.stringify(result));
  fs.writeFileSync(path.join(fixturesDir, multifileBinName), binBytes);
  console.log("wrote src/test/fixtures/" + multifileGltfName);
  console.log("wrote src/test/fixtures/" + multifileBinName);
}

// fixture-model-flat.zip — the multifile model with .gltf and .bin at the
// same zip level; the exporter's own buffer reference (a bare filename)
// exact-matches the zip entry directly.
{
  const gltfBytes = fs.readFileSync(path.join(fixturesDir, multifileGltfName));
  const binBytes = fs.readFileSync(path.join(fixturesDir, multifileBinName));
  const zipped = zipSync({
    [multifileGltfName]: new Uint8Array(gltfBytes),
    [multifileBinName]: new Uint8Array(binBytes),
  });
  fs.writeFileSync(path.join(fixturesDir, "fixture-model-flat.zip"), zipped);
  console.log("wrote src/test/fixtures/fixture-model-flat.zip");
}

// fixture-model-nested.zip — same content, but the .gltf and .bin sit in
// different subfolders. The exporter's buffer reference is still just the
// bare filename, so the exact-path lookup ("<bin-name>" at zip root) fails
// and resolution must fall back to the recursive basename search to find
// it at "buffers/<bin-name>" — this is what actually exercises the fallback
// path, not just the zip-decompression mechanics.
{
  const gltfBytes = fs.readFileSync(path.join(fixturesDir, multifileGltfName));
  const binBytes = fs.readFileSync(path.join(fixturesDir, multifileBinName));
  const zipped = zipSync({
    [`source/${multifileGltfName}`]: new Uint8Array(gltfBytes),
    [`buffers/${multifileBinName}`]: new Uint8Array(binBytes),
  });
  fs.writeFileSync(path.join(fixturesDir, "fixture-model-nested.zip"), zipped);
  console.log("wrote src/test/fixtures/fixture-model-nested.zip");
}

// fixture-unresolvable-texture.gltf — a minimal glTF whose material
// references "textures/missing-diffuse.png", deliberately never bundled
// alongside it anywhere. Hand-written JSON, not exported: GLTFExporter
// requires a real texture image to reference (needs canvas to encode one,
// unavailable here — see the FBX/Collada section above), but this fixture's
// whole point is a reference with nothing behind it, so there's no image to
// export in the first place. Used by ResourceResolver.test.ts to verify
// missing-resource tracking against a real committed fixture file rather
// than an inline-constructed one — the resolver-logic level is also as far
// as this scenario can be exercised in the automated suite: jsdom's <img>
// never fires load/error for any src, so routing this through the full
// GLTFLoader/Viewer.importModel pipeline hangs regardless of whether the
// reference resolves — confirmed and documented, not overlooked. The full
// pipeline behavior (missing-resource warning banner shown, model still
// renders) is verified manually in a real browser instead.
{
  const gltf = {
    asset: { version: "2.0" },
    scene: 0,
    scenes: [{ nodes: [0] }],
    nodes: [{ mesh: 0 }],
    meshes: [{ primitives: [{ attributes: { POSITION: 0 }, material: 0 }] }],
    materials: [{ pbrMetallicRoughness: { baseColorTexture: { index: 0 } } }],
    textures: [{ source: 0 }],
    images: [{ uri: "textures/missing-diffuse.png" }],
    accessors: [{ bufferView: 0, componentType: 5126, count: 3, type: "VEC3", max: [1, 1, 0], min: [0, 0, 0] }],
    bufferViews: [{ buffer: 0, byteOffset: 0, byteLength: 36 }],
    buffers: [
      {
        uri:
          "data:application/octet-stream;base64," +
          Buffer.from(new Float32Array([0, 0, 0, 1, 0, 0, 0, 1, 0]).buffer).toString("base64"),
        byteLength: 36,
      },
    ],
  };
  fs.writeFileSync(path.join(fixturesDir, "fixture-unresolvable-texture.gltf"), JSON.stringify(gltf));
  console.log("wrote src/test/fixtures/fixture-unresolvable-texture.gltf");
}
