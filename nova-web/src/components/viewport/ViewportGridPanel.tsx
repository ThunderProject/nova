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

const ViewportRow: React.FC<{
    viewports: ViewportModel[];
    removeViewport: (id: string) => void;
}> = ({ removeViewport, viewports }) => (
    <PanelGroup direction="horizontal" className={classes.panelGroup}>
        {
            viewports.map((viewportModel, i) => (
                <React.Fragment key={viewportModel.id}>
                    {
                        i > 0 && (<PanelResizeHandle className={classes.resizeHandleVertical} />)
                    }
                    <Panel className={classes.panel}>
                        <ViewportItem viewport={viewportModel} onClose={removeViewport} />
                    </Panel>
                </React.Fragment>
            ))
        }
    </PanelGroup>
);

export const ViewportGridPanel: React.FC<Props> = ({
    removeViewport,
    viewports,
}) => {
    const numViewports = viewports.length;
    if (numViewports === 0) {
        return <EmptyState />;
    }
    
    const columnsPerRow = 2;
    const rows = Array.from(
        { length: Math.ceil(numViewports / columnsPerRow) },
        (_, i) => viewports.slice(i * columnsPerRow, i * columnsPerRow + columnsPerRow)
    );
    
    if (rows.length === 1) {
        return <ViewportRow viewports={rows[0]} removeViewport={removeViewport} />;
    }
    
    return (
        <PanelGroup direction="vertical" className={classes.panelGroup}>
            {rows.map((row, rowIndex) => (
                <React.Fragment key={rowIndex}>
                    {
                        rowIndex > 0 && (<PanelResizeHandle className={classes.resizeHandleHorizontal} />)
                    }
                    <Panel defaultSize={100 / rows.length} className={classes.panel}>
                        <ViewportRow viewports={row} removeViewport={removeViewport} />
                    </Panel>
                </React.Fragment>
            ))}
        </PanelGroup>
    );
};

export default ViewportGridPanel;
