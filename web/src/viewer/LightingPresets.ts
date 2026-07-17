import * as THREE from "three";
import { HDRLoader } from "three/addons/loaders/HDRLoader.js";

export type LightingPreset = "day" | "night";

/**
 * Bundled, CC0-licensed HDRI environment maps (Poly Haven, 1k resolution,
 * public domain — attribution is not required but is good practice):
 *  - day.hdr:   "Kloofendal 43d Clear (Pure Sky)" by Greg Zaal
 *               https://polyhaven.com/a/kloofendal_43d_clear_puresky
 *  - night.hdr: "Kloppenheim 02 (Pure Sky)" by Greg Zaal / Jarod Guest
 *               https://polyhaven.com/a/kloppenheim_02_puresky
 */
export const LIGHTING_PRESET_PATHS: Record<LightingPreset, string> = {
  day: "environments/day.hdr",
  night: "environments/night.hdr",
};

/**
 * Parses an already-fetched .hdr buffer into an equirectangular texture —
 * mirrors ImportResolver.ts's parse-from-memory pattern used for the other
 * model loaders (GLTFLoader/FBXLoader/STLLoader `.parse(buffer)`). Pure
 * ArrayBuffer parsing with no image/DOM decode involved, so it runs the same
 * under jsdom as in a browser (verified empirically against the real
 * bundled files before writing fixture-based tests around this).
 *
 * Uses HDRLoader rather than the RGBELoader named in the spec — three.js
 * deprecated RGBELoader as of r180 in favor of HDRLoader, which is the exact
 * same RGBE parser under a new name.
 */
export function parseHdrBuffer(buffer: ArrayBuffer): THREE.DataTexture {
  const texture = new HDRLoader().createDataTexture(buffer);
  texture.mapping = THREE.EquirectangularReflectionMapping;
  return texture;
}

/**
 * GPU-side prefiltering step (PMREMGenerator) that turns a raw equirectangular
 * HDR texture into one suitable for scene.environment / material reflections.
 * Requires a live WebGL context, so — like Viewer's RendererLike — it's an
 * injectable seam: the real implementation wraps THREE.PMREMGenerator, tests
 * inject a stub instead of needing a real GPU.
 */
export interface EnvironmentProcessor {
  process(source: THREE.Texture): THREE.Texture;
  dispose(): void;
}

export class PMREMEnvironmentProcessor implements EnvironmentProcessor {
  private readonly generator: THREE.PMREMGenerator;

  constructor(renderer: THREE.WebGLRenderer) {
    this.generator = new THREE.PMREMGenerator(renderer);
    this.generator.compileEquirectangularShader();
  }

  process(source: THREE.Texture): THREE.Texture {
    return this.generator.fromEquirectangular(source).texture;
  }

  dispose(): void {
    this.generator.dispose();
  }
}

async function defaultLoadHdrBuffer(path: string): Promise<ArrayBuffer> {
  const response = await fetch(`${import.meta.env.BASE_URL}${path}`);
  return response.arrayBuffer();
}

/**
 * Owns loading, RGBE-parsing, PMREM-processing, and caching the two bundled
 * Lighting presets (see web/CONTEXT.md) — the seam issue #9 names
 * "LightingPresetManager". Viewer just calls load(preset) and binds the
 * result to scene.environment; this class does the fetch-once /
 * process-once / cache-forever work behind that, keyed per preset so
 * switching back to an already-loaded preset never reprocesses.
 */
export class LightingPresetManager {
  private readonly cache = new Map<LightingPreset, THREE.Texture>();

  constructor(
    private readonly environmentProcessor: EnvironmentProcessor,
    private readonly loadHdrBuffer: (path: string) => Promise<ArrayBuffer> = defaultLoadHdrBuffer,
  ) {}

  async load(preset: LightingPreset): Promise<THREE.Texture> {
    let envMap = this.cache.get(preset);
    if (!envMap) {
      const buffer = await this.loadHdrBuffer(LIGHTING_PRESET_PATHS[preset]);
      const hdrTexture = parseHdrBuffer(buffer);
      envMap = this.environmentProcessor.process(hdrTexture);
      hdrTexture.dispose();
      this.cache.set(preset, envMap);
    }
    return envMap;
  }

  dispose(): void {
    this.environmentProcessor.dispose();
    this.cache.forEach((texture) => texture.dispose());
    this.cache.clear();
  }
}
