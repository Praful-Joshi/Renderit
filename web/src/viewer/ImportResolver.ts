import * as THREE from "three";
import { GLTFLoader } from "three/addons/loaders/GLTFLoader.js";

/** Extensions this ticket's dispatcher recognizes. Later tickets (#12) add
 * more entries here — obj, fbx, stl, dae — without changing the shape of
 * resolveImportedFile itself. */
const EXTENSION_TO_LOADER: Record<string, "gltf"> = {
  gltf: "gltf",
  glb: "gltf",
};

export class UnsupportedFormatError extends Error {
  constructor(filename: string) {
    const supported = [...new Set(Object.keys(EXTENSION_TO_LOADER))].join(", ");
    super(`Unsupported file type: "${filename}". Supported formats: ${supported}`);
    this.name = "UnsupportedFormatError";
  }
}

function detectLoaderKind(filename: string): "gltf" | null {
  const ext = filename.toLowerCase().split(".").pop();
  return (ext && EXTENSION_TO_LOADER[ext]) || null;
}

const gltfLoader = new GLTFLoader();

/**
 * Format dispatcher: inspects the file's extension and routes to the
 * matching loader, or rejects with UnsupportedFormatError. GLTFLoader.parse
 * auto-detects GLB (binary) vs plain-JSON .gltf from the same ArrayBuffer,
 * so both extensions share one code path.
 */
export async function resolveImportedFile(file: File): Promise<THREE.Object3D> {
  const kind = detectLoaderKind(file.name);
  if (!kind) {
    throw new UnsupportedFormatError(file.name);
  }

  const buffer = await file.arrayBuffer();

  return new Promise<THREE.Object3D>((resolve, reject) => {
    gltfLoader.parse(buffer, "", (gltf) => resolve(gltf.scene), reject);
  });
}
