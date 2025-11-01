import { useEffect, useRef } from "react";
import { useRenderStore } from "../../stores/renderStore";

type Props = {
    viewportId: string;
};

export default function ViewportCanvas({ viewportId }: Props) {
    const canvasRef = useRef<HTMLCanvasElement>(null);
    const { getCtx, initRenderer } = useRenderStore();

    useEffect(() => {
        const canvas = canvasRef.current;
        if (!canvas) return;

        (async () => {
            await initRenderer(canvas);
            const ctx = getCtx();
            ctx?.registerView(viewportId, canvas);
        })();

        return () => {
            const ctx = getCtx();
            ctx?.unregisterView(viewportId);
        };
    }, [getCtx, initRenderer, viewportId]);

    return (
        <canvas
            ref={canvasRef}
            style={{
                background: "#0d0f1a",
                borderRadius: 8,
                display: "block",
                height: "100%",
                width: "100%",
            }}
        />
    );
}
