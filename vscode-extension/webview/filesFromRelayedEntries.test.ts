import { describe, expect, it } from "vitest";
import { filesFromRelayedEntries } from "./filesFromRelayedEntries";

describe("filesFromRelayedEntries", () => {
  it("reconstructs a File per entry with the basename and original bytes", async () => {
    const files = filesFromRelayedEntries([
      { relativePath: "MyModel/model.gltf", bytes: new TextEncoder().encode("{}") },
    ]);
    expect(files).toHaveLength(1);
    expect(files[0].name).toBe("model.gltf");
    expect(await files[0].text()).toBe("{}");
  });

  it("stamps webkitRelativePath with the full relative path, matching ImportFileSet's expected shape", () => {
    const [file] = filesFromRelayedEntries([
      { relativePath: "MyModel/textures/diffuse.png", bytes: new Uint8Array([1, 2, 3]) },
    ]);
    expect(file.webkitRelativePath).toBe("MyModel/textures/diffuse.png");
  });

  it("handles multiple entries independently, preserving order", () => {
    const files = filesFromRelayedEntries([
      { relativePath: "MyModel/model.gltf", bytes: new Uint8Array([1]) },
      { relativePath: "MyModel/model.bin", bytes: new Uint8Array([2]) },
    ]);
    expect(files.map((file) => file.name)).toEqual(["model.gltf", "model.bin"]);
    expect(files.map((file) => file.webkitRelativePath)).toEqual(["MyModel/model.gltf", "MyModel/model.bin"]);
  });
});
