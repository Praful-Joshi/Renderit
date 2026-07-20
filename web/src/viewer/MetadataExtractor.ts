import * as THREE from "three";
import type { ImportFileSet } from "./ImportFileSet";

export interface BoundingBoxSize {
  width: number;
  height: number;
  depth: number;
}

export interface ModelMetadata {
  fileName: string;
  format: string;
  fileSizeBytes: number;
  vertexCount: number;
  triangleCount: number;
  meshCount: number;
  materialCount: number;
  textureCount: number;
  boundingBoxSize: BoundingBoxSize;
}

export interface ExtractMetadataInput {
  object: THREE.Object3D;
  /** The pre-Auto-fit world-space bounding box — the same Box3 AutoFit
   * computes its scale/position from — so boundingBoxSize reports the
   * model's own file units, not the Auto-fit-normalized display size. */
  box: THREE.Box3;
  /** Every file involved in this import (the primary model file plus any
   * companion textures/materials/buffers) — summed for total file size. */
  files: ImportFileSet;
  fileName: string;
  format: string;
}

/**
 * Pure function over the imported Object3D graph and its source files — see
 * issue #16. Mesh/material/texture counts dedupe by object identity (a Set),
 * since materials and textures are commonly shared across meshes within one
 * model rather than duplicated per-mesh (mirrors the same
 * `Object.values(material)` texture-slot scan disposeObject3D already uses
 * in Viewer.ts, just counting instead of disposing).
 */
export function extractMetadata(input: ExtractMetadataInput): ModelMetadata {
  const { object, box, files, fileName, format } = input;

  let vertexCount = 0;
  let triangleCount = 0;
  let meshCount = 0;
  const materials = new Set<THREE.Material>();
  const textures = new Set<THREE.Texture>();

  object.traverse((child) => {
    const mesh = child as THREE.Mesh;
    if (!mesh.isMesh) return;

    meshCount++;
    const position = mesh.geometry.attributes.position;
    vertexCount += position.count;
    triangleCount += mesh.geometry.index ? mesh.geometry.index.count / 3 : position.count / 3;

    const meshMaterials = Array.isArray(mesh.material) ? mesh.material : [mesh.material];
    for (const material of meshMaterials) {
      materials.add(material);
      for (const value of Object.values(material)) {
        if (value instanceof THREE.Texture) textures.add(value);
      }
    }
  });

  let fileSizeBytes = 0;
  for (const file of files.values()) fileSizeBytes += file.size;

  const size = box.getSize(new THREE.Vector3());

  return {
    fileName,
    format,
    fileSizeBytes,
    vertexCount,
    triangleCount,
    meshCount,
    materialCount: materials.size,
    textureCount: textures.size,
    boundingBoxSize: { width: size.x, height: size.y, depth: size.z },
  };
}
