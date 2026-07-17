import * as THREE from "three";
import { GLTFLoader } from "three/addons/loaders/GLTFLoader.js";
import { OBJLoader } from "three/addons/loaders/OBJLoader.js";
import { MTLLoader } from "three/addons/loaders/MTLLoader.js";
import { FBXLoader } from "three/addons/loaders/FBXLoader.js";
import { STLLoader } from "three/addons/loaders/STLLoader.js";
import { ColladaLoader } from "three/addons/loaders/ColladaLoader.js";

type LoaderKind = "gltf" | "obj" | "fbx" | "stl" | "collada";

const EXTENSION_TO_LOADER: Record<string, LoaderKind> = {
  gltf: "gltf",
  glb: "gltf",
  obj: "obj",
  fbx: "fbx",
  stl: "stl",
  dae: "collada",
};

export class UnsupportedFormatError extends Error {
  constructor(filename: string) {
    const supported = [...new Set(Object.keys(EXTENSION_TO_LOADER))].join(", ");
    super(`Unsupported file type: "${filename}". Supported formats: ${supported}`);
    this.name = "UnsupportedFormatError";
  }
}

function detectLoaderKind(filename: string): LoaderKind | null {
  const ext = filename.toLowerCase().split(".").pop();
  return (ext && EXTENSION_TO_LOADER[ext]) || null;
}

// Shared singletons — safe because none of these mutate persistent instance
// state during parse(): GLTFLoader/ColladaLoader build a fresh internal
// parser per call, FBXLoader's actual per-parse work happens on a
// freshly-constructed FBXTreeParser, and STLLoader's parse() touches no
// `this` at all. OBJLoader is the one exception (see loadObj below).
const gltfLoader = new GLTFLoader();
const mtlLoader = new MTLLoader();
const fbxLoader = new FBXLoader();
const stlLoader = new STLLoader();
const colladaLoader = new ColladaLoader();

async function loadGltf(file: File): Promise<THREE.Object3D> {
  const buffer = await file.arrayBuffer();
  return new Promise<THREE.Object3D>((resolve, reject) => {
    gltfLoader.parse(buffer, "", (gltf) => resolve(gltf.scene), reject);
  });
}

async function loadObj(file: File, companions: File[]): Promise<THREE.Object3D> {
  // A fresh OBJLoader per call — its `materials` field is mutable state set
  // via setMaterials(), and a shared instance would leak a companion .mtl
  // from a previous import into a later OBJ-only import that has none.
  const objLoader = new OBJLoader();
  const mtlFile = companions.find((f) => f.name.toLowerCase().endsWith(".mtl"));
  if (mtlFile) {
    objLoader.setMaterials(mtlLoader.parse(await mtlFile.text(), ""));
  }
  return objLoader.parse(await file.text());
}

async function loadFbx(file: File): Promise<THREE.Object3D> {
  const buffer = await file.arrayBuffer();
  return fbxLoader.parse(buffer, "");
}

async function loadStl(file: File): Promise<THREE.Object3D> {
  const buffer = await file.arrayBuffer();
  const geometry = stlLoader.parse(buffer);
  return new THREE.Mesh(geometry, new THREE.MeshStandardMaterial());
}

async function loadCollada(file: File): Promise<THREE.Object3D> {
  const result = colladaLoader.parse(await file.text(), "");
  if (!result) {
    throw new Error(`Failed to parse Collada file: "${file.name}"`);
  }
  return result.scene;
}

/**
 * Format dispatcher: finds the file whose extension is recognized (the
 * "primary" model file — any others are companion resources, e.g. a .mtl
 * alongside an .obj) and routes it to the matching loader, or rejects with
 * UnsupportedFormatError if none of the given files match a known format.
 *
 * Auto-orientation is not implemented here — it's inherent to the loaders
 * themselves (FBXLoader/ColladaLoader read each format's own up-axis
 * metadata and self-correct; OBJLoader/STLLoader have no such metadata and
 * are used as-is). See docs/adr/0003-auto-orientation-scope.md.
 */
export async function resolveImportedFile(files: File[]): Promise<THREE.Object3D> {
  const withKind = files.map((file) => ({ file, kind: detectLoaderKind(file.name) }));
  const primaryEntry = withKind.find((entry) => entry.kind !== null);
  if (!primaryEntry) {
    throw new UnsupportedFormatError(files[0]?.name ?? "(no file)");
  }
  const { file: primary, kind } = primaryEntry;
  const companions = files.filter((f) => f !== primary);

  switch (kind) {
    case "gltf":
      return loadGltf(primary);
    case "obj":
      return loadObj(primary, companions);
    case "fbx":
      return loadFbx(primary);
    case "stl":
      return loadStl(primary);
    case "collada":
      return loadCollada(primary);
    case null:
      // Unreachable: `primary` was only selected because detectLoaderKind
      // matched it above. Satisfies TS's exhaustiveness/return-path check.
      throw new UnsupportedFormatError(primary.name);
  }
}
