import {WebGPUEngine} from "@babylonjs/core/Engines/webgpuEngine";
import {Color4} from "@babylonjs/core";
import {Scene} from "@babylonjs/core/scene";
import {ArcRotateCamera} from "@babylonjs/core/Cameras/arcRotateCamera";
import {Vector3} from "@babylonjs/core/Maths/math.vector";
import {HemisphericLight} from "@babylonjs/core/Lights/hemisphericLight";
import {MeshBuilder} from "@babylonjs/core/Meshes/meshBuilder";
import {NovaApi} from "../nova_api/NovaApi.ts";

type RenderConfig = {
    clearColor: Color4;
};

export class RenderContext {
    readonly #gpuRenderer: WebGPUEngine;
    readonly #renderConfig: RenderConfig = {
        clearColor: new Color4(0, 0, 0, 0)
    };
    #scene: Scene | null = null;
    readonly #canvas: HTMLCanvasElement;

    constructor(canvas: HTMLCanvasElement) {
        this.#canvas = canvas;
        this.#gpuRenderer = new WebGPUEngine(
            this.#canvas,
            {
                antialias: true,
                powerPreference: "high-performance",
                forceFallbackAdapter: false,
                enableAllFeatures: true,
                enableGPUDebugMarkers: false,
            }
        );
    }

    render() {
        const novaApi = new NovaApi();
        novaApi.dicom_open("worldor").then(r => console.log(r));

        if(!this.#scene) {
            return;
        }

        this.#scene.clearColor = this.#renderConfig.clearColor;

        const camera = new ArcRotateCamera("camera", Math.PI / 2, Math.PI / 2.5, 4, Vector3.Zero(), this.#scene);
        camera.attachControl(this.#canvas, true);

        const resize = () => this.#gpuRenderer.resize();
        window.addEventListener("resize", resize);

        new HemisphericLight("light", new Vector3(0, 1, 0), this.#scene);
        const box = MeshBuilder.CreateBox("box", {}, this.#scene);

        this.#gpuRenderer.runRenderLoop(() => {
            box.rotation.y += 0.01;
            this.#scene?.render();
        });
    }

    setClearColor(color: Color4): void {
        this.#renderConfig.clearColor = color;
    }

    async init(): Promise<void> {
        await this.#gpuRenderer.initAsync();
        this.#scene = new Scene(this.#gpuRenderer);
    }

    dispose() {
        //window.removeEventListener("resize", resize);
        this.#gpuRenderer.dispose();
    }
}