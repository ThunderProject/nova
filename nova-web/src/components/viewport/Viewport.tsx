import {type FC, useEffect, useRef} from "react";
import {Color4} from "@babylonjs/core";
import {RenderContext} from "../../renderer/RenderContext.ts";

const Viewport: FC = () => {
    const canvasRef = useRef<HTMLCanvasElement>(null);

    useEffect(() => {
        const init = async () => {
            if(!canvasRef.current) {
                return;
            }

            const preventWheelScroll = (e: WheelEvent) => {
                e.preventDefault();
            };
            canvasRef.current.addEventListener("wheel", preventWheelScroll, { passive: false });

            const renderCtx = new RenderContext(canvasRef.current);
            await renderCtx.init();
            renderCtx.setClearColor(new Color4(0.05, 0.05, 0.1, 1));
            renderCtx.render();

            return () => {
                canvasRef.current?.removeEventListener("wheel", preventWheelScroll);
                renderCtx.dispose();
            };
        }
        void init();
    });

    return (
        <div style={{
            width: "100%",
            height: "100%",
            border: "1px solid #2c2f39",
            backgroundColor: "#0d0f1a",
            borderRadius: "8px",
            position: "relative",
            overflow: "hidden",
        }}>
            <canvas
                ref={canvasRef}
                style={{
                    width: "100%",
                    height: '100%',
                    display: "block"
                }}
            />
        </div>
    );
};

export default Viewport;