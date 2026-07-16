import { describe, expect, it } from "vitest";
import * as THREE from "three";
import { AUTO_FIT_TARGET_SIZE, computeAutoFit } from "./AutoFit";

describe("computeAutoFit", () => {
  it("scales a model so its largest dimension matches the target size", () => {
    // 2 x 4 x 2 box -> largest dimension is 4 (the height)
    const box = new THREE.Box3(new THREE.Vector3(-1, -2, -1), new THREE.Vector3(1, 2, 1));

    const { scale } = computeAutoFit(box);

    expect(scale).toBeCloseTo(AUTO_FIT_TARGET_SIZE / 4);
  });

  it("computes a position that centers the box at the origin once scaled", () => {
    // Same box as above, offset so its center sits at (5, 5, 5).
    const box = new THREE.Box3(new THREE.Vector3(4, 3, 4), new THREE.Vector3(6, 7, 6));

    const { scale, position } = computeAutoFit(box);

    // center(5,5,5) * scale + position must equal the origin.
    const resultingCenter = new THREE.Vector3(5, 5, 5).multiplyScalar(scale).add(position);
    expect(resultingCenter.x).toBeCloseTo(0);
    expect(resultingCenter.y).toBeCloseTo(0);
    expect(resultingCenter.z).toBeCloseTo(0);
  });

  it("matches hand-computed values for a known offset box", () => {
    // 2x4x2 box centered at (5,5,5): maxDimension=4, scale=TARGET/4,
    // position = -scale * center.
    const box = new THREE.Box3(new THREE.Vector3(4, 3, 4), new THREE.Vector3(6, 7, 6));

    const { scale, position } = computeAutoFit(box);
    const expectedScale = AUTO_FIT_TARGET_SIZE / 4;

    expect(scale).toBeCloseTo(expectedScale);
    expect(position.x).toBeCloseTo(-expectedScale * 5);
    expect(position.y).toBeCloseTo(-expectedScale * 5);
    expect(position.z).toBeCloseTo(-expectedScale * 5);
  });

  it("does not divide by zero for a degenerate (zero-size) box", () => {
    const box = new THREE.Box3(new THREE.Vector3(0, 0, 0), new THREE.Vector3(0, 0, 0));

    const { scale } = computeAutoFit(box);

    expect(Number.isFinite(scale)).toBe(true);
  });
});
