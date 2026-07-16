import * as THREE from "three";
import { OrbitControls } from "three/addons/controls/OrbitControls.js";

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

export interface ViewerOptions {
  canvas: HTMLCanvasElement;
  /** Injectable for tests; defaults to a real THREE.WebGLRenderer. */
  renderer?: RendererLike;
  width?: number;
  height?: number;
}

const DEFAULT_CAMERA_POSITION = new THREE.Vector3(0, 1.4, 4.2);
const DEFAULT_CONTROLS_TARGET = new THREE.Vector3(0, 0.5, 0);

export class Viewer {
  readonly scene: THREE.Scene;
  readonly camera: THREE.PerspectiveCamera;
  readonly controls: OrbitControls;

  private readonly renderer: RendererLike;
  private readonly initialCameraPosition: THREE.Vector3;
  private readonly initialControlsTarget: THREE.Vector3;
  private animationFrameId: number | null = null;

  constructor(options: ViewerOptions) {
    const width = options.width || options.canvas.clientWidth || 1;
    const height = options.height || options.canvas.clientHeight || 1;

    this.scene = new THREE.Scene();
    this.scene.background = new THREE.Color(0x14161a);

    this.camera = new THREE.PerspectiveCamera(45, width / height, 0.1, 100);
    this.camera.position.copy(DEFAULT_CAMERA_POSITION);

    this.renderer = options.renderer ?? new THREE.WebGLRenderer({ canvas: options.canvas, antialias: true });
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

    this.addPlaceholderContent();
  }

  /** Placeholder-lit object so camera/lighting behavior is verifiable before
   * real model import lands (see issue #11). */
  private addPlaceholderContent(): void {
    const ambient = new THREE.AmbientLight(0xffffff, 0.4);
    const directional = new THREE.DirectionalLight(0xffffff, 1.2);
    directional.position.set(3, 5, 2);
    this.scene.add(ambient, directional);

    const geometry = new THREE.TorusKnotGeometry(0.6, 0.2, 128, 32);
    const material = new THREE.MeshStandardMaterial({
      color: 0x5b8cff,
      roughness: 0.35,
      metalness: 0.4,
    });
    const mesh = new THREE.Mesh(geometry, material);
    mesh.position.y = 0.5;
    this.scene.add(mesh);
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
  }
}
