import * as THREE from "three";

/** Every imported model is normalized to this bounding-box max dimension,
 * chosen to fill the majority of the viewport under the fixed default
 * camera framing established in Viewer's constructor. */
export const AUTO_FIT_TARGET_SIZE = 2.0;

export interface AutoFitResult {
  scale: number;
  position: THREE.Vector3;
}

/**
 * Computes the uniform scale and position that, applied to an object with
 * the given world-space bounding box, centers it at the origin with its
 * largest dimension equal to AUTO_FIT_TARGET_SIZE.
 */
export function computeAutoFit(box: THREE.Box3): AutoFitResult {
  const size = box.getSize(new THREE.Vector3());
  const maxDimension = Math.max(size.x, size.y, size.z);
  const scale = maxDimension > 0 ? AUTO_FIT_TARGET_SIZE / maxDimension : 1;

  const center = box.getCenter(new THREE.Vector3());
  const position = center.multiplyScalar(-scale);

  return { scale, position };
}
