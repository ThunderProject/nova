import { render, screen, fireEvent } from "@testing-library/react";
import { vi } from "vitest";
import React from "react";
import ViewportShell from "./ViewportShell";

vi.mock("@mantine/core", async () => {
    const actual = await vi.importActual<typeof import("@mantine/core")>("@mantine/core");

    type MockProps = React.PropsWithChildren<React.HTMLAttributes<HTMLDivElement>>;

    const MockDiv = ({ children, ...props }: MockProps) => (
        <div {...props}>{children}</div>
    );

    return {
        ...actual,
        ActionIcon: ({
                         children,
                         onClick,
                     }: React.PropsWithChildren<{ onClick?: () => void }>) => (
            <button data-testid="fullscreen-btn" onClick={onClick}>
                {children}
            </button>
        ),
        CloseButton: ({ onClick }: { onClick?: () => void }) => (
            <button data-testid="close-btn" onClick={onClick}>x</button>
        ),
        Group: (props: MockProps) => <MockDiv data-testid="group" {...props} />,
        Paper: (props: MockProps) => <MockDiv data-testid="paper" {...props} />,
        Text: ({ children }: React.PropsWithChildren<object>) => <span>{children}</span>,
        Tooltip: ({ children }: React.PropsWithChildren<object>) => <>{children}</>,
    };
});

vi.mock("@tabler/icons-react", () => ({
    IconArrowsDiagonal: () => <div data-testid="enter-icon" />,
    IconArrowsDiagonalMinimize: () => <div data-testid="exit-icon" />,
}));

const mockSetSelectedViewportId = vi.fn();
vi.mock("../../stores/viewerTypes.ts", () => ({
    useViewerStore: () => ({
        selectedViewportId: "vp1",
        setSelectedViewportId: mockSetSelectedViewportId,
    }),
}));

vi.mock("../../lib/Logger.ts", () => ({
    logger: {
        debug: vi.fn(),
        error: vi.fn(),
    },
}));

describe("ViewportShell", () => {
    const onClose = vi.fn();

    beforeEach(() => {
        vi.clearAllMocks();
        Object.defineProperty(document, "fullscreenElement", {
            value: null,
            writable: true,
        });
        document.exitFullscreen = vi.fn().mockResolvedValue(undefined);
        HTMLElement.prototype.requestFullscreen = vi.fn().mockResolvedValue(undefined);
    });

    it("renders with title and children", () => {
        render(
            <ViewportShell id="vp1" title="Test Viewport" onClose={onClose}>
                <div>Child Content</div>
            </ViewportShell>
        );

        expect(screen.getByText("Test Viewport")).toBeInTheDocument();
        expect(screen.getByText("Child Content")).toBeInTheDocument();
    });

    it("calls onClose when close button is clicked", () => {
        render(<ViewportShell id="vp1" onClose={onClose}>child</ViewportShell>);
        fireEvent.click(screen.getByTestId("close-btn"));
        expect(onClose).toHaveBeenCalledWith("vp1");
    });

    it("requests fullscreen when fullscreen icon clicked", async () => {
        render(<ViewportShell id="vp1">child</ViewportShell>);
        fireEvent.click(screen.getByTestId("fullscreen-btn"));
        expect(HTMLElement.prototype.requestFullscreen).toHaveBeenCalled();
    });

    it("updates state on fullscreenchange", () => {
        render(<ViewportShell id="vp1">child</ViewportShell>);
        const event = new Event("fullscreenchange");
        document.dispatchEvent(event);
        expect(screen.getByTestId("paper")).toBeInTheDocument();
    });

    it("selects viewport when header clicked", () => {
        render(<ViewportShell id="vp1">child</ViewportShell>);
        const group = screen.getAllByTestId("group")[0];
        fireEvent.click(group);
        expect(mockSetSelectedViewportId).toHaveBeenCalledWith("vp1");
    });
});
