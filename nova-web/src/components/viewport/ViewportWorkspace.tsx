import {type JSX, useEffect, useMemo, useRef, useState} from "react";
import {
    ActionIcon,
    Tabs,

} from "@mantine/core";
import { IconLayoutGrid, IconLayoutList } from "@tabler/icons-react";
import { useViewerStore, type ViewportModel } from "../../stores/viewerTypes.ts";
import { logger } from "../../lib/Logger.ts";
import ViewportShell from "../ViewportShell/ViewportShell.tsx";
import ViewportGridPanel from "../ViewportGridPanel/ViewportGridPanel.tsx";
import { GridManager } from "../../layout/GridManager.ts";
import LayoutToolbar from "../LayoutToolbar/LayoutToolbar.tsx";
import ViewportCanvas from "./ViewPortCanvas";
import classes from "./ViewportWorkspace.module.css";

type LayoutKind = "tabs" | "grid";

const layoutOptions = [
    { icon: <IconLayoutList size={14} />, label: "Tabs", value: "tabs" },
    { icon: <IconLayoutGrid size={14} />, label: "Grid", value: "grid" },
] satisfies { value: LayoutKind; label: string; icon: JSX.Element }[];

export default function ViewerWorkspace() {
    const {
        addViewport,
        layout,
        order,
        removeViewport,
        setLayout,
        setViewportVisible,
        viewports,
    } = useViewerStore();

    const items = order.map((id) => viewports[id]).filter(Boolean);

    const gridRef = useRef(
        new GridManager({ columns: 2, rows: 2 })
    )

    const grid = gridRef.current;
    const [activeTab, setActiveTab] = useState(0);

    const visibleIds = useMemo(() => items.map(v => v.id), [items]);
    useMemo(() => {
        for (const id of visibleIds) {
            if (!grid.indexOf(id))  {
                grid.add(id);
            }
        }

        for (const id of grid.groups().flat()) {
            if (!visibleIds.includes(id)) {
                grid.remove(id);
            }
        }
    }, [grid, visibleIds]);

    useEffect(() => {
        const tabCount = grid.groups().length;
        const clamped = Math.min(activeTab, Math.max(0, tabCount - 1));

        if (clamped !== activeTab) {
            setActiveTab(clamped);
            grid.setActiveTab(clamped);
        }
    }, [grid, items.length, activeTab]);

    useEffect(() => {
        grid.setActiveTab(activeTab);
    }, [activeTab, grid]);

    const Toolbar = (
        <LayoutToolbar<LayoutKind>
            layout={layout}
            onChangeLayout={(layoutKind) => {
                if (items.length === 0) {
                    logger.warn("Cannot change layout before adding a viewport");
                    return;
                }
                setLayout(layoutKind);
            }}
            layoutOptions={layoutOptions}
            actionLabel="Add viewport"
            actions={[
                {
                    label: "2D View",
                    onClick: () => {
                        const id = crypto.randomUUID();
                        addViewport({
                            id,
                            layers: [
                                {
                                    displayableId: "dummyCT",
                                    id: "main",
                                    params: { kind: "windowing", value: { center: 40, width: 400 } },
                                    rendererKind: "CT",
                                },
                            ],
                            title: "2D View",
                            visible: true,
                        });
                    },
                },
            ]}
            disabled={items.length === 0}
        />
    );

    const TabsLayout = useMemo(() => {
        if (items.length === 0) {
            return null;
        }

        return (
            <Tabs
                defaultValue={items[0]?.id}
                keepMounted={false}
                classNames={{
                    panel: classes.Panel,
                    root: classes.tabsRoot,
                }}
                onChange={(tabId) => {
                    order.forEach((id) => setViewportVisible(id, id === tabId));
                }}
            >
                <Tabs.List>
                    {items.map((viewportModel) => (
                        <Tabs.Tab
                            key={viewportModel.id}
                            value={viewportModel.id}
                            rightSection={
                                <ActionIcon
                                    size="xs"
                                    variant="subtle"
                                    component="div"
                                    onClick={(e) => {
                                        e.stopPropagation();
                                        grid.remove(viewportModel.id);
                                        removeViewport(viewportModel.id);
                                    }}
                                >
                                    ✕
                                </ActionIcon>
                            }
                        >
                            {viewportModel.title}
                        </Tabs.Tab>
                    ))}
                </Tabs.List>

                {items.map((vp) => (
                    <Tabs.Panel key={vp.id} value={vp.id} className={classes.tabsPanel}>
                        <ViewportShell
                            id={vp.id}
                            title={vp.title}
                            onClose={() => removeViewport(vp.id)}
                        >
                            <ViewportCanvas viewportId={vp.id} />
                        </ViewportShell>
                    </Tabs.Panel>
                ))}
            </Tabs>
        );
    }, [grid, items, order, removeViewport, setViewportVisible]);

    const GridLayout = useMemo(() => {
        if (items.length === 0) {
          return null;
        }

        const groups = grid.groups().map((ids) =>
            ids.map((id) => viewports[id]).filter(Boolean) as ViewportModel[]
        );

        return (
            <Tabs
                value={String(activeTab)}
                onChange={(value) => {
                    const idx = Number(value ?? 0);
                    setActiveTab(idx);
                    grid.setActiveTab(idx);
                }}
                keepMounted={false}
                classNames={{
                    panel: classes.Panel,
                    root: classes.tabsRoot,
                }}
            >
                <Tabs.List>
                    {groups.map((_, i) => (
                        <Tabs.Tab key={i} value={String(i)}>
                            Grid {i + 1}
                        </Tabs.Tab>
                    ))}
                </Tabs.List>

                {groups.map((group, i) => (
                    <Tabs.Panel key={i} value={String(i)} className={classes.Panel}>
                        <ViewportGridPanel
                            viewports={group}
                            removeViewport={(id) => { grid.remove(id); removeViewport(id); }}
                        />
                    </Tabs.Panel>
                ))}
            </Tabs>
        );
    }, [activeTab, grid, items.length, removeViewport, viewports]);

    const body = useMemo(() => {
        if (items.length === 0) {
            return (
                <div className={classes.emptyState}>
                    No viewports yet — add one above.
                </div>
            );
        }

        return layout === "grid" ? GridLayout : TabsLayout;
    }, [layout, GridLayout, TabsLayout, items.length]);

    return (
        <div className={classes.workspace}>
            {Toolbar}
            <div className={classes.content}>{body}</div>
        </div>
    );
}
