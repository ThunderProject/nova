import {useEffect, useRef, useState} from "react";
import {DoubleNavbar} from "../DoubleNavbar.tsx";
import ViewportPanel from "../components/viewport/ViewportPanel.tsx";
import {ResizableDoubleNavbar} from "../components/ResizableDoubleNavbar/ResizableDoubleNavbar.tsx";

export default function Viewer() {
    const canvasRef = useRef<HTMLCanvasElement | null>(null);

    useEffect(() => {
        const canvas = canvasRef.current;
        if (!canvas) {
            return;
        }

        const ctx = canvas.getContext('2d');
        if (ctx) {
            ctx.fillStyle = '#1e1e1e';
            ctx.fillRect(0, 0, canvas.width, canvas.height);
        }
    }, []);

    const [isOpen, setIsOpen] = useState(true);
    return (
        <div
            style=  {{
                display: 'flex',
                height: 'calc(100vh - 100px)',
            }}
        >
            <ResizableDoubleNavbar>
                <DoubleNavbar />
            </ResizableDoubleNavbar>

            <div style={{ flex: 1, padding: 8 }}>
                {isOpen ? (
                    <ViewportPanel onClose={() => setIsOpen(false)} />
                ) : (
                    <button onClick={() => setIsOpen(true)}>Reopen Viewport</button>
                )}
            </div>
        </div>
    );
}