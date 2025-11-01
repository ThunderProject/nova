import {WebGPUEngine} from "@babylonjs/core/Engines/webgpuEngine";
import {Color4, Mesh} from "@babylonjs/core";
import {Scene} from "@babylonjs/core/scene";
import {ArcRotateCamera} from "@babylonjs/core/Cameras/arcRotateCamera";
import {Vector3} from "@babylonjs/core/Maths/math.vector";
import {HemisphericLight} from "@babylonjs/core/Lights/hemisphericLight";
import {MeshBuilder} from "@babylonjs/core/Meshes/meshBuilder";
import {logger} from "../lib/Logger.ts";

type RenderConfig = {
    clearColor: Color4;
};

export type ViewHandle = {
    id: string,
    canvas: HTMLCanvasElement,
    camera: ArcRotateCamera
}

export class RenderContext {
    #engine: WebGPUEngine | null = null;
    readonly #renderConfig: RenderConfig = {
        clearColor: new Color4(0, 0, 0, 0),
    };
    #scene: Scene | null = null;
    #views = new Map<string, ViewHandle>();
    #running = false;
    #box!: Mesh;

    async init(canvas: HTMLCanvasElement): Promise<void> {
        if(this.#engine) {
            return;
        }

        this.#engine = new WebGPUEngine(canvas, {
            adaptToDeviceRatio: true,
            antialias: true,
            enableAllFeatures: true,
            powerPreference: "high-performance",
        })

        await this.#engine.initAsync();
        this.#scene = new Scene(this.#engine);
        this.#scene.clearColor = new Color4(0.05, 0.05, 0.1, 1);

        new HemisphericLight("light", new Vector3(0, 1, 0), this.#scene);
        this.#box = MeshBuilder.CreateBox("box", { size: 1 }, this.#scene);

        window.addEventListener("resize", () => this.resize());
        this.#scene.debugLayer.show();
    }

    registerView(id: string, canvas: HTMLCanvasElement): boolean {
        try {
            this.#engine?.resize();

            if (!this.#engine) {
                logger.error("Unable to register view: engine not initialized");
                return false;
            }

            if (!this.#scene) {
                logger.error("Unable to register view: scene not initialized");
                return false;
            }

            const camera = new ArcRotateCamera(
                "camera-" + id,
                Math.PI / 4,
                Math.PI / 3,
                6,
                Vector3.Zero(),
                this.#scene
            );
            camera.setTarget(Vector3.Zero());

            this.#engine.registerView(canvas, camera);
            camera.attachControl(canvas, true);

            this.#views.set(id, {camera, canvas, id});

            if (!this.#running && this.#views.size > 0) {
                this.startRenderLoop();
            }

            return true;
        }
        catch (err) {
            logger.error(`RenderContext failed to register view. Reason: ${err}`);
            return false;
        }
    }

    unregisterView(id: string) {
        const handle = this.#views.get(id);

        if(handle && this.#scene) {
            this.#engine?.unRegisterView(handle.canvas);
            handle.camera.dispose();
            this.#views.delete(id);

            if(this.#views.size === 0) {
                this.#engine?.stopRenderLoop();
            }
        }
    }

    render() {
        logger.debug("Rendering");
        this.#box.rotation.y += 0.001;
        this.#scene?.render();
    }

    resize(): void {
        logger.debug("Resizing");
        try {
            this.#engine?.resize();
        } catch (err) {
            console.warn("RenderContext resize failed", err);
        }
    }

    setClearColor(color: Color4): void {
        this.#renderConfig.clearColor = color;

        if (this.#scene) {
            this.#scene.clearColor = color;
        }
    }


    private startRenderLoop() {
        logger.debug("Starting render loop");

        if(this.#running  || !this.#engine || !this.#scene) {
            logger.debug("Render loop already running");
            return;
        }

        this.#running = true;
        this.#engine.runRenderLoop(() => {
            this.render()
        });
    }

    dispose() {
        logger.debug("Disposing render context");
        this.#engine?.stopRenderLoop();
        this.#scene?.dispose();
        this.#engine?.dispose();
        this.#scene = null;
        this.#engine = null;
        this.#views.clear();
        this.#running = false;
    }
}