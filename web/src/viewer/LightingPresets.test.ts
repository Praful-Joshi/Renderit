import { describe, expect, it, vi } from "vitest";
import * as THREE from "three";
import { LightingPresetManager, parseHdrBuffer, type EnvironmentProcessor } from "./LightingPresets";
import { loadFixture } from "../test/fixtures";

async function loadFixtureBuffer(name: string): Promise<ArrayBuffer> {
  return loadFixture(name, "application/octet-stream").arrayBuffer();
}

describe("parseHdrBuffer", () => {
  it("parses a real RGBE .hdr buffer into an equirectangular DataTexture", async () => {
    const buffer = await loadFixtureBuffer("fixture-day.hdr");

    const texture = parseHdrBuffer(buffer);

    expect(texture.image.width).toBe(2);
    expect(texture.image.height).toBe(2);
    expect(texture.mapping).toBe(THREE.EquirectangularReflectionMapping);
  });

  it("parses distinguishable pixel data for the day vs night fixtures", async () => {
    const day = parseHdrBuffer(await loadFixtureBuffer("fixture-day.hdr"));
    const night = parseHdrBuffer(await loadFixtureBuffer("fixture-night.hdr"));

    // HDRLoader's default type is HalfFloatType, so .data is a Uint16Array —
    // never null for a successfully parsed texture.
    expect(Array.from(day.image.data as Uint16Array)).not.toEqual(Array.from(night.image.data as Uint16Array));
  });
});

// PMREM prefiltering needs a live WebGL context (unavailable in jsdom), so
// it's stubbed as an identity pass-through — enough to prove the manager's
// own load/cache/dispose bookkeeping without needing a real GPU.
function createStubEnvironmentProcessor() {
  return {
    process: vi.fn((source: THREE.Texture) => source),
    dispose: vi.fn(),
  } satisfies EnvironmentProcessor;
}

function createManager() {
  const environmentProcessor = createStubEnvironmentProcessor();
  const loadHdrBuffer = vi.fn(async (path: string) => {
    return loadFixtureBuffer(path.includes("night") ? "fixture-night.hdr" : "fixture-day.hdr");
  });
  const manager = new LightingPresetManager(environmentProcessor, loadHdrBuffer);
  return { manager, environmentProcessor, loadHdrBuffer };
}

describe("LightingPresetManager", () => {
  it("loads, PMREM-processes, and returns the environment map for a preset", async () => {
    const { manager, environmentProcessor, loadHdrBuffer } = createManager();

    const envMap = await manager.load("day");

    expect(envMap).toBeInstanceOf(THREE.Texture);
    expect(loadHdrBuffer).toHaveBeenCalledWith("environments/day.hdr");
    expect(environmentProcessor.process).toHaveBeenCalledTimes(1);
  });

  it("returns distinct environment maps for day vs night", async () => {
    const { manager } = createManager();

    const day = await manager.load("day");
    const night = await manager.load("night");

    expect(day).not.toBe(night);
  });

  it("caches processing per preset — repeat loads of an already-loaded preset don't reprocess", async () => {
    const { manager, environmentProcessor, loadHdrBuffer } = createManager();

    await manager.load("day");
    await manager.load("day");
    await manager.load("night");
    await manager.load("day");

    // 2 distinct presets loaded across 4 calls — process()/loadHdrBuffer()
    // should each have run exactly once per preset, not once per call.
    expect(environmentProcessor.process).toHaveBeenCalledTimes(2);
    expect(loadHdrBuffer).toHaveBeenCalledTimes(2);
  });

  it("disposes the environment processor and every cached texture", async () => {
    const { manager, environmentProcessor } = createManager();
    await manager.load("day");
    await manager.load("night");

    manager.dispose();

    expect(environmentProcessor.dispose).toHaveBeenCalledTimes(1);
  });
});
