import React, {useEffect, useRef, useState} from "react";
import {
    ActionIcon,
    CloseButton,
    Group,
    Paper,
    Text, Tooltip,
} from "@mantine/core";
import {IconArrowsDiagonal, IconArrowsDiagonalMinimize} from "@tabler/icons-react";
import { useViewerStore } from "../../stores/viewerTypes";
import {logger} from "../../lib/Logger.ts";
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
    const containerRef = useRef<HTMLDivElement>(null);
    const [isFullscreen, setIsFullscreen] = useState(false);

    function selectViewport(element: HTMLElement) {
        if (!element.closest("button")) {
            setSelectedViewportId(id);
        }
    }

    const enterFullScreen = async (elem: HTMLDivElement) => {
        try {
            await elem.requestFullscreen();
            logger.debug("Entered fullscreen mode");
        } catch (err) {
            logger.error(`Failed to enter fullscreen: ${err}`);
        }
    };

    const exitFullScreen = async () => {
        try {
            await document.exitFullscreen();
            logger.debug("Exited Fullscreen mode");
        } catch (err) {
            logger.error(`Failed to exit fullscreen: ${err}`);
        }
    };

    const toggleFullscreen = async () => {
        const elem = containerRef.current;
        if (!elem)  {
            return;
        }

        if (document.fullscreenElement) {
            await exitFullScreen();
        }
        else {
            await enterFullScreen(elem);
        }
    };

    useEffect(() => {
        const handleFullscreenChange = () => {
            const elem = containerRef.current;
            const active = document.fullscreenElement === elem;
            setIsFullscreen(active);
        };

        document.addEventListener("fullscreenchange", handleFullscreenChange);

        return () => {
            document.removeEventListener("fullscreenchange", handleFullscreenChange);
        };
    }, []);

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
                    label= {isFullscreen ? "Exit fullscreen" : "Enter fullscreen"}
                    openDelay={300}
                >
                    <ActionIcon
                        size="sm"
                        variant="subtle"
                        onClick={(e) => {
                            e.stopPropagation();
                            void toggleFullscreen();
                        }}
                    >
                        {
                            isFullscreen
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
            ref={containerRef}
            withBorder
            radius="sm"
            className={`${classes.viewport} ${isSelected ? classes.selected : ""}`}
        >
            <Header/>
            <Body/>
        </Paper>
    );
}
