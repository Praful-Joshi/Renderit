import * as THREE from "three";
import { OrbitControls } from "three/addons/controls/OrbitControls.js";
import { computeAutoFit } from "./AutoFit";
import { resolveImportedFile } from "./ImportResolver";
import { buildImportFileSet } from "./ImportFileSet";
import { LightingPresetManager, PMREMEnvironmentProcessor, type LightingPreset } from "./LightingPresets";

export type { LightingPreset } from "./LightingPresets";

/**
 * Minimal surface Viewer needs from a renderer. Lets tests inject a stub
 * instead of a real THREE.WebGLRenderer, which requires a live WebGL context
 * unavailable in jsdom/CI.
 */
export interface RendererLike {
  domElement: HTMLElement;
  setSize(width: number, height: number): void;
  setPixelRatio(ratio: number): void;
  render(scene: THREE.Scene, camera: THREE.Camera): void;
  dispose(): void;
}

export interface ImportModelResult {
  model: THREE.Object3D;
  /** Texture/material references that couldn't be resolved anywhere in the
   * import — the model still loads, just missing those maps. */
  missingResources: string[];
}

/** Whether the visible scene.background is the fixed neutral studio color
 * (default) or the active Lighting preset's own HDRI image. Independent of
 * Lighting preset, which always drives scene.environment regardless of this
 * setting — see web/CONTEXT.md's "Background mode". */
export type BackgroundMode = "studio" | "hdri";

export interface ViewerOptions {
  canvas: HTMLCanvasElement;
  /** Injectable for tests; defaults to a real THREE.WebGLRenderer. */
  renderer?: RendererLike;
  /** Injectable for tests (PMREM prefiltering needs a live WebGL context);
   * defaults to a LightingPresetManager built from the renderer above.
   * Required if `renderer` is a non-WebGLRenderer stub — there's no real
   * renderer to build the default manager's PMREM processor from then. */
  lightingPresetManager?: LightingPresetManager;
  width?: number;
  height?: number;
}

const DEFAULT_CAMERA_POSITION = new THREE.Vector3(0, 1.4, 4.2);
// The origin — every imported model is Auto-fit to be centered here
// (see AutoFit.ts), so the fixed camera framing below always applies
// regardless of which model is currently loaded.
const DEFAULT_CONTROLS_TARGET = new THREE.Vector3(0, 0, 0);

export class Viewer {
  readonly scene: THREE.Scene;
  readonly camera: THREE.PerspectiveCamera;
  readonly controls: OrbitControls;

  private readonly renderer: RendererLike;
  private readonly lightingPresetManager: LightingPresetManager;
  private readonly initialCameraPosition: THREE.Vector3;
  private readonly initialControlsTarget: THREE.Vector3;
  private animationFrameId: number | null = null;
  private currentModel: THREE.Object3D | null = null;
  private currentModelObjectUrls: string[] = [];
  private readonly studioBackgroundColor: THREE.Color;
  private activeLightingPreset: LightingPreset | null = null;
  private activeBackgroundMode: BackgroundMode = "studio";

  constructor(options: ViewerOptions) {
    const width = options.width || options.canvas.clientWidth || 1;
    const height = options.height || options.canvas.clientHeight || 1;

    this.scene = new THREE.Scene();
    // Studio is the default Background mode — see setBackgroundMode() and
    // web/CONTEXT.md's "Background mode". Per-instance, not a shared
    // module-level constant, since THREE.Color is mutable.
    this.studioBackgroundColor = new THREE.Color(0x14161a);
    this.scene.background = this.studioBackgroundColor;

    this.camera = new THREE.PerspectiveCamera(45, width / height, 0.1, 100);
    this.camera.position.copy(DEFAULT_CAMERA_POSITION);

    let lightingPresetManager = options.lightingPresetManager;
    if (options.renderer) {
      this.renderer = options.renderer;
    } else {
      const webglRenderer = new THREE.WebGLRenderer({ canvas: options.canvas, antialias: true });
      this.renderer = webglRenderer;
      lightingPresetManager ??= new LightingPresetManager(new PMREMEnvironmentProcessor(webglRenderer));
    }
    if (!lightingPresetManager) {
      throw new Error("Viewer: lightingPresetManager must be provided alongside a custom renderer");
    }
    this.lightingPresetManager = lightingPresetManager;

    this.renderer.setPixelRatio(Math.min(window.devicePixelRatio ?? 1, 2));
    this.renderer.setSize(width, height);

    this.controls = new OrbitControls(this.camera, options.canvas);
    this.controls.target.copy(DEFAULT_CONTROLS_TARGET);
    this.controls.enableDamping = true;
    this.controls.dampingFactor = 0.08;
    this.controls.enablePan = false;
    this.controls.autoRotateSpeed = 1.2;
    // Sync controls' internal spherical state with the target above before
    // autoRotate is enabled, so this call can't nudge the camera and the
    // "initial" pose captured below is exactly deterministic.
    this.controls.update();

    this.initialCameraPosition = this.camera.position.clone();
    this.initialControlsTarget = this.controls.target.clone();

    this.controls.autoRotate = true;

    // Idle auto-rotate plays until the visitor first takes control, then stays
    // off permanently for the session — { once: true } enforces "permanently"
    // without needing extra state to track whether it already fired.
    options.canvas.addEventListener(
      "pointerdown",
      () => {
        this.controls.autoRotate = false;
      },
      { once: true },
    );
  }

  /** Currently active Lighting preset, or null before the first
   * setLightingPreset() call resolves. */
  get lightingPreset(): LightingPreset | null {
    return this.activeLightingPreset;
  }

  /** Currently active Background mode — see setBackgroundMode(). */
  get backgroundMode(): BackgroundMode {
    return this.activeBackgroundMode;
  }

  /**
   * Switches scene.environment (lighting/reflections) to the given HDRI
   * Lighting preset, delegating the fetch/parse/PMREM/cache work to
   * lightingPresetManager (see LightingPresets.ts). Independent of Background
   * mode, but if hdri Background mode is active, the visible background is
   * re-applied too so it keeps matching whichever preset is now lighting the
   * scene.
   */
  async setLightingPreset(preset: LightingPreset): Promise<void> {
    this.scene.environment = await this.lightingPresetManager.load(preset);
    this.activeLightingPreset = preset;
    this.applyBackgroundMode();
  }

  /**
   * Switches the visible scene.background between the fixed studio color
   * (studio, the default) and the active Lighting preset's own HDRI image
   * (hdri) — see web/CONTEXT.md's "Background mode". Independent of
   * setLightingPreset(): this only changes what's visible behind the model,
   * never scene.environment (lighting/reflections stay driven by the
   * Lighting preset regardless of this setting). If hdri is requested before
   * any Lighting preset has loaded, falls back to the studio color until a
   * preset resolves.
   */
  setBackgroundMode(mode: BackgroundMode): void {
    this.activeBackgroundMode = mode;
    this.applyBackgroundMode();
  }

  private applyBackgroundMode(): void {
    // scene.environment is the single source of truth for "the active
    // Lighting preset's texture" — setLightingPreset is the only place that
    // writes it, so hdri mode can read it back directly rather than
    // mirroring it into a second field.
    this.scene.background =
      this.activeBackgroundMode === "hdri" && this.scene.environment
        ? this.scene.environment
        : this.studioBackgroundColor;
  }

  /** Used for both the bundled showcase model on startup and visitor
   * imports, so there's exactly one code path for "a model is now on
   * screen" — rejecting leaves the current model untouched. `source` may be
   * a single file, multiple flat files (e.g. an .obj with its .mtl), a zip,
   * or a folder-picker/drag-drop file list — see ImportFileSet. */
  async importModel(source: File | File[]): Promise<ImportModelResult> {
    const fileSet = await buildImportFileSet(source);
    const { object, missingResources, objectUrls } = await resolveImportedFile(fileSet);

    const box = new THREE.Box3().setFromObject(object);
    const { scale, position } = computeAutoFit(box);
    object.scale.setScalar(scale);
    object.position.copy(position);

    if (this.currentModel) {
      this.scene.remove(this.currentModel);
      disposeObject3D(this.currentModel);
      this.currentModelObjectUrls.forEach((url) => URL.revokeObjectURL(url));
    }

    this.scene.add(object);
    this.currentModel = object;
    this.currentModelObjectUrls = objectUrls;

    return { model: object, missingResources };
  }

  resetView(): void {
    // Suppress auto-rotate for this one sync call — otherwise update() applies
    // an auto-rotate tick (based on wall-clock time elapsed since the last
    // call) on top of the restored pose, making the reset inexact.
    const wasAutoRotating = this.controls.autoRotate;
    this.controls.autoRotate = false;
    this.camera.position.copy(this.initialCameraPosition);
    this.controls.target.copy(this.initialControlsTarget);
    this.controls.update();
    this.controls.autoRotate = wasAutoRotating;
  }

  resize(width: number, height: number): void {
    this.camera.aspect = width / height;
    this.camera.updateProjectionMatrix();
    this.renderer.setSize(width, height);
  }

  start(): void {
    if (this.animationFrameId !== null) return;
    const tick = () => {
      this.controls.update();
      this.renderer.render(this.scene, this.camera);
      this.animationFrameId = requestAnimationFrame(tick);
    };
    this.animationFrameId = requestAnimationFrame(tick);
  }

  stop(): void {
    if (this.animationFrameId !== null) {
      cancelAnimationFrame(this.animationFrameId);
      this.animationFrameId = null;
    }
  }

  dispose(): void {
    this.stop();
    this.controls.dispose();
    this.renderer.dispose();
    this.lightingPresetManager.dispose();
  }
}

/** Frees GPU-side geometry/material/texture buffers so replacing the
 * currently displayed model doesn't leak resources across imports. */
function disposeObject3D(root: THREE.Object3D): void {
  root.traverse((child) => {
    const mesh = child as THREE.Mesh;
    if (!mesh.isMesh) return;

    mesh.geometry.dispose();

    const materials = Array.isArray(mesh.material) ? mesh.material : [mesh.material];
    for (const material of materials) {
      for (const value of Object.values(material)) {
        if (value instanceof THREE.Texture) value.dispose();
      }
      material.dispose();
    }
  });
}
