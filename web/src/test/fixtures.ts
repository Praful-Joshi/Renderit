import { readFileSync } from "node:fs";
import { resolve } from "node:path";
import * as THREE from "three";

export function loadFixture(name: string, type: string): File {
  const buffer = readFileSync(resolve(process.cwd(), "src/test/fixtures", name));
  return new File([buffer], name, { type });
}

export function countMeshes(root: THREE.Object3D): number {
  let count = 0;
  root.traverse((child) => {
    if ((child as THREE.Mesh).isMesh) count++;
  });
  return count;
}
