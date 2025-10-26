import React from "react";
import { render, screen } from "@testing-library/react";
import { vi } from "vitest";
import type { ViewportModel } from "../../stores/viewerTypes";
import { ViewportGridPanel } from "./ViewportGridPanel";
import "@testing-library/jest-dom";

vi.mock("./ViewportShell", () => ({
    default: ({ children }: { children: React.ReactNode }) => (
        <div data-testid="viewport-shell">{children}</div>
    ),
}));

vi.mock("./ViewPortCanvas", () => ({
    default: ({ viewportId }: { viewportId: string }) => (
        <div data-testid={`viewport-canvas-${viewportId}`}>Canvas {viewportId}</div>
    ),
}));

const createViewports = (count: number): ViewportModel[] =>
    Array.from({ length: count }, (_, i) => (
        {
            id: `vp${i + 1}`,
            title: `Viewport ${i + 1}`,
        }
    )
) as ViewportModel[];

describe("ViewportGridPanel", () => {
    const removeViewport = vi.fn();

    afterEach(() => {
        vi.clearAllMocks();
    });

    it("renders empty state when there are no viewports", () => {
        render(<ViewportGridPanel viewports={[]} removeViewport={removeViewport} />);
        expect(screen.getByText(/no viewports/i)).toBeInTheDocument();
    });

    it("renders a single viewport", () => {
        const viewports = createViewports(1);
        render(<ViewportGridPanel viewports={viewports} removeViewport={removeViewport} />);
        expect(screen.getByTestId("viewport-canvas-vp1")).toBeInTheDocument();
    });

    it("renders two viewports in one horizontal row", () => {
        const viewports = createViewports(2);
        render(<ViewportGridPanel viewports={viewports} removeViewport={removeViewport} />);
        const canvases = screen.getAllByTestId(/viewport-canvas-/);
        expect(canvases).toHaveLength(2);
    });

    it("renders three viewports in two rows", () => {
        const viewports = createViewports(3);
        render(<ViewportGridPanel viewports={viewports} removeViewport={removeViewport} />);
        const canvases = screen.getAllByTestId(/viewport-canvas-/);
        expect(canvases).toHaveLength(3);
    });

    it("renders four viewports in two rows", () => {
        const viewports = createViewports(4);
        render(<ViewportGridPanel viewports={viewports} removeViewport={removeViewport} />);
        const canvases = screen.getAllByTestId(/viewport-canvas-/);
        expect(canvases).toHaveLength(4);
    });
});
