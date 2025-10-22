import React from "react";
import {
    CloseButton,
    Group,
    Paper,
    Text,
} from "@mantine/core";
import { useViewerStore } from "../../stores/viewerTypes";
import classes from "./ViewportShell.module.css";

type ViewportShellProps = {
    id: string;
    title?: string;
    onClose?: (id: string) => void;
    onToggleMaximize?: (id: string) => void;
    headerExtra?: React.ReactNode;
    children: React.ReactNode;
};

export default function ViewportShell({
                                          children,
                                          headerExtra,
                                          id,
                                          onClose,
                                          title = "Viewport",
                                      }: ViewportShellProps) {

    const { selectedViewportId, setSelectedViewportId } = useViewerStore();
    const isSelected = selectedViewportId === id;

    function selectViewport(element: HTMLElement) {
        if (!element.closest("button")) {
            setSelectedViewportId(id);
        }
    }

    const Header = () => (
        <Group
            className={classes.header}
            onClick={(e) => selectViewport(e.target as HTMLElement)}
        >
            <Group gap={6}>
                <Text size="sm" fw={500} lineClamp={1}>
                    {title}
                </Text>
                {headerExtra}
            </Group>

            <Group gap={4}>
                <CloseButton
                    size="sm"
                    onClick={(e) => {
                        e.stopPropagation();
                        onClose?.(id);
                    }}
                />
            </Group>
        </Group>
    );

    const Body = () => <div className={`${classes.body} `}>{children}</div>

    return (
        <Paper
            withBorder
            radius="sm"
            className={`${classes.viewport} ${isSelected ? classes.selected : ""}`}
        >
            <Header/>
            <Body/>
        </Paper>
    );
}
