import * as THREE from "three";
import type { ImportFileSet } from "./ImportFileSet";

export interface ResourceResolution {
  /** Bound to this one import — resolves reference URLs against `files`.
   * Pass to every loader involved in this import (gltf/obj/mtl/fbx/dae). */
  manager: THREE.LoadingManager;
  /** References that couldn't be resolved by either strategy, in the order
   * first encountered. Populated as the manager's loaders run, so only
   * meaningful after the import's parse has completed. */
  missingResources: string[];
  /** Blob URLs created for resolved references, for revoking once the
   * model they belong to is replaced. */
  objectUrls: string[];
}

/**
 * Builds a LoadingManager whose URL modifier resolves every external
 * resource reference a loader makes against `files` — textures, a glTF's
 * external .bin buffer, anything fetched by URL — against the exporter's
 * original relative path first, falling back to a case-insensitive
 * recursive filename search across the whole import. This is the
 * format-agnostic resolution layer every loader shares, rather than each
 * loader special-casing file lookup.
 */
export function createResourceResolver(files: ImportFileSet): ResourceResolution {
  const missingResources: string[] = [];
  const objectUrls: string[] = [];
  const manager = new THREE.LoadingManager();

  manager.setURLModifier((url) => {
    if (url.startsWith("data:") || url.startsWith("blob:")) {
      return url;
    }

    const match = findExact(files, url) ?? findByBasename(files, url);

    if (!match) {
      if (!missingResources.includes(url)) missingResources.push(url);
      return url;
    }

    const objectUrl = URL.createObjectURL(match);
    objectUrls.push(objectUrl);
    return objectUrl;
  });

  return { manager, missingResources, objectUrls };
}

function normalizePath(path: string): string {
  return path.replace(/\\/g, "/").replace(/^\.\//, "");
}

function basename(path: string): string {
  const normalized = normalizePath(path);
  const index = normalized.lastIndexOf("/");
  return index === -1 ? normalized : normalized.slice(index + 1);
}

function findExact(files: ImportFileSet, referencePath: string): File | undefined {
  return files.get(normalizePath(referencePath));
}

function findByBasename(files: ImportFileSet, referencePath: string): File | undefined {
  const target = basename(referencePath).toLowerCase();
  for (const [path, file] of files) {
    if (basename(path).toLowerCase() === target) return file;
  }
  return undefined;
}
