import { useState } from "react";
import { DoubleNavbar } from "../DoubleNavbar.tsx";
import { ResizableDoubleNavbar } from "../components/ResizableDoubleNavbar/ResizableDoubleNavbar.tsx";
import ViewerWorkspace from "../components/viewport/ViewportWorkspace.tsx";

export default function Viewer() {
    const [isOpen, setIsOpen] = useState(true);

    return (
        <div
            style={{
                display: "flex",
                flexDirection: "row",
                height: "calc(100vh - 100px)",
                minHeight: 0,
                overflow: "hidden",
            }}
        >
            <ResizableDoubleNavbar>
                <DoubleNavbar />
            </ResizableDoubleNavbar>

            <div style={{ display: "flex", flex: 1, flexDirection: "column", minHeight: 0, overflow: "hidden" }}>
                {isOpen ? (
                    <ViewerWorkspace />
                ) : (
                    <button onClick={() => setIsOpen(true)}>Reopen Viewer</button>
                )}
            </div>
        </div>

    );
}
