import { readFileSync } from "node:fs";
import { resolve } from "node:path";
import * as THREE from "three";

export function loadFixture(name: string, type: string): File {
  const buffer = readFileSync(resolve(process.cwd(), "src/test/fixtures", name));
  return new File([buffer], name, { type });
}

/** Loads several fixtures at once into an ImportFileSet-shaped Map, keyed
 * by filename (flat — no subfolder structure). `type` defaults to a generic
 * binary MIME type since tests generally don't depend on it. */
export function loadFixtureSet(names: string[], type = "application/octet-stream"): Map<string, File> {
  return new Map(names.map((name) => [name, loadFixture(name, type)]));
}

/** Loads a fixture and sets .webkitRelativePath, simulating a file picked
 * via a folder-select input (<input type=file webkitdirectory>) or a
 * dropped folder — `folderName` is the container folder's own name, which
 * ImportFileSet strips off (see stripCommonRootSegment). */
export function loadFolderFixture(folderName: string, subPath: string, fixtureName: string, type: string): File {
  const file = loadFixture(fixtureName, type);
  Object.defineProperty(file, "webkitRelativePath", { value: `${folderName}/${subPath}` });
  return file;
}

export function countMeshes(root: THREE.Object3D): number {
  let count = 0;
  root.traverse((child) => {
    if ((child as THREE.Mesh).isMesh) count++;
  });
  return count;
}
