import React from "react";
import {
    PanelGroup,
    Panel,
    PanelResizeHandle,
} from "react-resizable-panels";
import { type ViewportModel } from "../../stores/viewerTypes";
import ViewportShell from "./ViewportShell";
import ViewportCanvas from "./ViewPortCanvas";
import classes from "./ViewportGridPanel.module.css";

type Props = {
    viewports: ViewportModel[];
    removeViewport: (id: string) => void;
    onResetLayout?: () => void;
};

const EmptyState: React.FC = () => (
    <div className={classes.emptyState}>No viewports</div>
);

const ViewportItem: React.FC<{
    viewport: ViewportModel;
    onClose: (id: string) => void;
}> = ({ onClose, viewport }) => (
    <ViewportShell
        id={viewport.id}
        title={viewport.title}
        onClose={() => onClose(viewport.id)}
    >
        <ViewportCanvas viewportId={viewport.id} />
    </ViewportShell>
);

export const ViewportGridPanel: React.FC<Props> = ({
                                                       removeViewport,
                                                       viewports,
                                                   }) => {
    const numViewports = viewports.length;

    if (numViewports === 0) {
        return <EmptyState />;
    }

    if (numViewports === 1) {
        return <ViewportItem viewport={viewports[0]} onClose={removeViewport} />;
    }

    if (numViewports === 2) {
        return (
            <PanelGroup direction="horizontal" className={classes.panelGroup}>
                {viewports.map((vp, i) => (
                    <React.Fragment key={vp.id}>
                        {i > 0 && (
                            <PanelResizeHandle className={classes.resizeHandleVertical} />
                        )}
                        <Panel className={classes.panel}>
                            <ViewportItem viewport={vp} onClose={removeViewport} />
                        </Panel>
                    </React.Fragment>
                ))}
            </PanelGroup>
        );
    }

    if (numViewports === 3) {
        const topRow = viewports.slice(0, 2);
        const bottom = viewports[2];

        return (
            <PanelGroup direction="vertical" className={classes.panelGroup}>
                <Panel defaultSize={50} className={classes.panel}>
                    <PanelGroup direction="horizontal" className={classes.panelGroup}>
                        {topRow.map((vp, i) => (
                            <React.Fragment key={vp.id}>
                                {i > 0 && (
                                    <PanelResizeHandle className={classes.resizeHandleVertical} />
                                )}
                                <Panel>
                                    <ViewportItem viewport={vp} onClose={removeViewport} />
                                </Panel>
                            </React.Fragment>
                        ))}
                    </PanelGroup>
                </Panel>

                <PanelResizeHandle className={classes.resizeHandleHorizontal} />
                <Panel className={classes.panel}>
                    <ViewportItem viewport={bottom} onClose={removeViewport} />
                </Panel>
            </PanelGroup>
        );
    }

    const topRow = viewports.slice(0, 2);
    const bottomRow = viewports.slice(2, 4);

    const renderRow = (row: ViewportModel[]) => (
        <PanelGroup direction="horizontal" className={classes.panelGroup}>
            {row.map((vp, i) => (
                <React.Fragment key={vp.id}>
                    {i > 0 && (
                        <PanelResizeHandle className={classes.resizeHandleVertical} />
                    )}
                    <Panel className={classes.panel}>
                        <ViewportItem viewport={vp} onClose={removeViewport} />
                    </Panel>
                </React.Fragment>
            ))}
        </PanelGroup>
    );

    return (
        <PanelGroup direction="vertical" className={classes.panelGroup}>
            <Panel defaultSize={50} className={classes.panel}>{renderRow(topRow)}</Panel>
            <PanelResizeHandle className={classes.resizeHandleHorizontal} />
            <Panel className={classes.panel}>{renderRow(bottomRow)}</Panel>
        </PanelGroup>
    );
};

export default ViewportGridPanel;
