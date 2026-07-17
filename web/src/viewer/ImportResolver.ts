import * as THREE from "three";
import { GLTFLoader } from "three/addons/loaders/GLTFLoader.js";
import { OBJLoader } from "three/addons/loaders/OBJLoader.js";
import { MTLLoader } from "three/addons/loaders/MTLLoader.js";
import { FBXLoader } from "three/addons/loaders/FBXLoader.js";
import { STLLoader } from "three/addons/loaders/STLLoader.js";
import { ColladaLoader } from "three/addons/loaders/ColladaLoader.js";
import type { ImportFileSet } from "./ImportFileSet";
import { createResourceResolver } from "./ResourceResolver";

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

export interface ResolvedImport {
  object: THREE.Object3D;
  /** References (texture/material paths) that couldn't be resolved against
   * the imported files — surfaced so the UI can show a clear message rather
   * than the model silently rendering with maps missing. */
  missingResources: string[];
  /** Blob URLs created to satisfy resolved references, owned by the caller
   * to revoke once this model is replaced. */
  objectUrls: string[];
}

// Every format but STL can reference external resources (textures, a
// companion .mtl), so their loaders need a per-import LoadingManager scoped
// to this import's own file set — hence fresh instances per call, not
// shared singletons. STL has no such references and needs no manager.
async function loadGltf(file: File, manager: THREE.LoadingManager): Promise<THREE.Object3D> {
  const buffer = await file.arrayBuffer();
  return new Promise<THREE.Object3D>((resolve, reject) => {
    new GLTFLoader(manager).parse(buffer, "", (gltf) => resolve(gltf.scene), reject);
  });
}

async function loadObj(file: File, files: ImportFileSet, manager: THREE.LoadingManager): Promise<THREE.Object3D> {
  const objLoader = new OBJLoader(manager);
  const mtlEntry = [...files.entries()].find(
    ([, candidate]) => candidate !== file && candidate.name.toLowerCase().endsWith(".mtl"),
  );
  if (mtlEntry) {
    const materials = new MTLLoader(manager).parse(await mtlEntry[1].text(), "");
    objLoader.setMaterials(materials);
  }
  return objLoader.parse(await file.text());
}

async function loadFbx(file: File, manager: THREE.LoadingManager): Promise<THREE.Object3D> {
  const buffer = await file.arrayBuffer();
  return new FBXLoader(manager).parse(buffer, "");
}

async function loadStl(file: File): Promise<THREE.Object3D> {
  const buffer = await file.arrayBuffer();
  const geometry = new STLLoader().parse(buffer);
  return new THREE.Mesh(geometry, new THREE.MeshStandardMaterial());
}

async function loadCollada(file: File, manager: THREE.LoadingManager): Promise<THREE.Object3D> {
  const result = new ColladaLoader(manager).parse(await file.text(), "");
  if (!result) {
    throw new Error(`Failed to parse Collada file: "${file.name}"`);
  }
  return result.scene;
}

/**
 * Format dispatcher: finds the file whose extension is recognized (the
 * "primary" model file — any others are companion resources, e.g. a .mtl
 * alongside an .obj, or textures referenced from anywhere in `files`) and
 * routes it to the matching loader, or rejects with UnsupportedFormatError
 * if none of the given files match a known format.
 *
 * Auto-orientation is not implemented here — it's inherent to the loaders
 * themselves (FBXLoader/ColladaLoader read each format's own up-axis
 * metadata and self-correct; OBJLoader/STLLoader have no such metadata and
 * are used as-is). See docs/adr/0003-auto-orientation-scope.md.
 */
export async function resolveImportedFile(files: ImportFileSet): Promise<ResolvedImport> {
  const withKind = [...files.values()].map((file) => ({ file, kind: detectLoaderKind(file.name) }));
  const primary = withKind.find((entry) => entry.kind !== null);
  if (!primary) {
    throw new UnsupportedFormatError(withKind[0]?.file.name ?? "(no file)");
  }
  const { file: primaryEntry, kind } = primary;
  const { manager, missingResources, objectUrls } = createResourceResolver(files);

  let object: THREE.Object3D;
  switch (kind) {
    case "gltf":
      object = await loadGltf(primaryEntry, manager);
      break;
    case "obj":
      object = await loadObj(primaryEntry, files, manager);
      break;
    case "fbx":
      object = await loadFbx(primaryEntry, manager);
      break;
    case "stl":
      object = await loadStl(primaryEntry);
      break;
    case "collada":
      object = await loadCollada(primaryEntry, manager);
      break;
    case null:
      // Unreachable: primaryEntry was only selected because detectLoaderKind
      // matched it above. Satisfies TS's exhaustiveness/return-path check.
      throw new UnsupportedFormatError(primaryEntry.name);
  }

  return { object, missingResources, objectUrls };
}
