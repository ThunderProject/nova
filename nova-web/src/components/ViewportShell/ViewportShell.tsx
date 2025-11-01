import React from "react";
import {
    ActionIcon,
    CloseButton,
    Group,
    Paper,
    Text, Tooltip,
} from "@mantine/core";
import { useFullscreen } from '@mantine/hooks';
import {IconArrowsDiagonal, IconArrowsDiagonalMinimize} from "@tabler/icons-react";
import { useViewerStore } from "../../stores/viewerTypes.ts";
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
    const { fullscreen, ref, toggle } = useFullscreen();

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
                <Tooltip
                    label= {fullscreen ? "Exit fullscreen" : "Enter fullscreen"}
                    openDelay={300}
                >
                    <ActionIcon
                        size="sm"
                        variant="subtle"
                        onClick= {toggle}
                    >
                        {
                            fullscreen
                                ? (<IconArrowsDiagonalMinimize size={16} />)
                                : (<IconArrowsDiagonal size={16} />)
                        }
                    </ActionIcon>
                </Tooltip>

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
            ref={ref}
            radius="sm"
            className={`${classes.viewport} ${isSelected ? classes.selected : ""}`}
        >
            <Header/>
            <Body/>
        </Paper>
    );
}
