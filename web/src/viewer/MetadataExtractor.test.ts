import { describe, expect, it } from "vitest";
import * as THREE from "three";
import { extractMetadata } from "./MetadataExtractor";
import type { ImportFileSet } from "./ImportFileSet";

function boxMesh(material: THREE.Material): THREE.Mesh {
  return new THREE.Mesh(new THREE.BoxGeometry(2, 2, 2), material);
}

function fileSet(entries: [string, number][]): ImportFileSet {
  return new Map(entries.map(([name, size]) => [name, new File([new Uint8Array(size)], name)]));
}

describe("extractMetadata", () => {
  it("passes fileName and format through unchanged", () => {
    const object = new THREE.Group();
    const box = new THREE.Box3().setFromObject(object);

    const metadata = extractMetadata({
      object,
      box,
      files: fileSet([]),
      fileName: "model.glb",
      format: "glTF/GLB",
    });

    expect(metadata.fileName).toBe("model.glb");
    expect(metadata.format).toBe("glTF/GLB");
  });

  it("counts vertices and triangles for a single mesh (THREE.BoxGeometry: 24 verts, 12 tris)", () => {
    const object = boxMesh(new THREE.MeshStandardMaterial());
    const box = new THREE.Box3().setFromObject(object);

    const metadata = extractMetadata({ object, box, files: fileSet([]), fileName: "box.glb", format: "glTF/GLB" });

    expect(metadata.vertexCount).toBe(24);
    expect(metadata.triangleCount).toBe(12);
    expect(metadata.meshCount).toBe(1);
  });

  it("sums vertex/triangle/mesh counts across multiple meshes", () => {
    const group = new THREE.Group();
    group.add(boxMesh(new THREE.MeshStandardMaterial()));
    group.add(boxMesh(new THREE.MeshStandardMaterial()));
    const box = new THREE.Box3().setFromObject(group);

    const metadata = extractMetadata({
      object: group,
      box,
      files: fileSet([]),
      fileName: "group.glb",
      format: "glTF/GLB",
    });

    expect(metadata.vertexCount).toBe(48);
    expect(metadata.triangleCount).toBe(24);
    expect(metadata.meshCount).toBe(2);
  });

  it("counts a material shared across meshes once, not once per mesh", () => {
    const sharedMaterial = new THREE.MeshStandardMaterial();
    const group = new THREE.Group();
    group.add(boxMesh(sharedMaterial));
    group.add(boxMesh(sharedMaterial));
    const box = new THREE.Box3().setFromObject(group);

    const metadata = extractMetadata({
      object: group,
      box,
      files: fileSet([]),
      fileName: "group.glb",
      format: "glTF/GLB",
    });

    expect(metadata.materialCount).toBe(1);
  });

  it("counts distinct materials, including multi-material meshes (mesh.material as an array)", () => {
    const object = new THREE.Mesh(new THREE.BoxGeometry(2, 2, 2), [
      new THREE.MeshStandardMaterial(),
      new THREE.MeshStandardMaterial(),
    ]);
    const box = new THREE.Box3().setFromObject(object);

    const metadata = extractMetadata({ object, box, files: fileSet([]), fileName: "box.glb", format: "glTF/GLB" });

    expect(metadata.materialCount).toBe(2);
  });

  it("counts distinct textures across a material's texture slots, deduping shared textures", () => {
    const sharedTexture = new THREE.Texture();
    const material = new THREE.MeshStandardMaterial({
      map: sharedTexture,
      normalMap: new THREE.Texture(),
      roughnessMap: sharedTexture, // same texture reused in a second slot
    });
    const object = boxMesh(material);
    const box = new THREE.Box3().setFromObject(object);

    const metadata = extractMetadata({ object, box, files: fileSet([]), fileName: "box.glb", format: "glTF/GLB" });

    expect(metadata.textureCount).toBe(2);
  });

  it("reports zero counts for an object with no meshes", () => {
    const object = new THREE.Group();
    const box = new THREE.Box3().setFromObject(object);

    const metadata = extractMetadata({ object, box, files: fileSet([]), fileName: "empty.glb", format: "glTF/GLB" });

    expect(metadata.vertexCount).toBe(0);
    expect(metadata.triangleCount).toBe(0);
    expect(metadata.meshCount).toBe(0);
    expect(metadata.materialCount).toBe(0);
    expect(metadata.textureCount).toBe(0);
  });

  it("computes bounding-box dimensions from the given (pre-Auto-fit) Box3", () => {
    // 2x4x2 box (matches AutoFit.test.ts's known-offset-box convention).
    const box = new THREE.Box3(new THREE.Vector3(-1, -2, -1), new THREE.Vector3(1, 2, 1));

    const metadata = extractMetadata({
      object: new THREE.Group(),
      box,
      files: fileSet([]),
      fileName: "box.glb",
      format: "glTF/GLB",
    });

    expect(metadata.boundingBoxSize.width).toBeCloseTo(2);
    expect(metadata.boundingBoxSize.height).toBeCloseTo(4);
    expect(metadata.boundingBoxSize.depth).toBeCloseTo(2);
  });

  it("sums byte sizes of every file in the import, not just the primary model file", () => {
    const object = new THREE.Group();
    const box = new THREE.Box3().setFromObject(object);

    const metadata = extractMetadata({
      object,
      box,
      files: fileSet([
        ["model.obj", 1000],
        ["model.mtl", 200],
        ["texture.png", 5000],
      ]),
      fileName: "model.obj",
      format: "OBJ",
    });

    expect(metadata.fileSizeBytes).toBe(6200);
  });
});
