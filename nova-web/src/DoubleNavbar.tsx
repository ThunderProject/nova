import {
    IconZoomInArea,
    IconPointer,
    IconAdjustmentsHorizontal,
    IconRotate,
    IconRulerMeasure,
} from '@tabler/icons-react';
import { Title, Tooltip, UnstyledButton } from '@mantine/core';
import classes from './DoubleNavbar.module.css';
import novaLogo from './assets/nova_icon.png';

import { useViewerToolsStore } from './stores/ViewerToolStore';
import type { ViewerTool } from './stores/ViewerToolStore';
import CTWindowingToolSettings from "./components/CTWindowingToolSettings/CTWindowingToolSettings.tsx";

//mock components
const ZoomToolSettings = () => <p>Zoom tool settings</p>;
const MeasurementList = () => <p>Measurement results</p>;

export function DoubleNavbar() {
    const { activeTool, setTool } = useViewerToolsStore();

    const viewerTools = [
        { icon: IconZoomInArea, label: 'Zoom' },
        { icon: IconPointer, label: 'Pan' },
        { icon: IconAdjustmentsHorizontal, label: 'Windowing' },
        { icon: IconRotate, label: 'Rotate' },
        { icon: IconRulerMeasure, label: 'Measure' },
    ];

    const toolIcons = viewerTools.map((tool) => (
        <Tooltip key={tool.label} label={tool.label} position="right" withArrow>
            <UnstyledButton
                onClick={() => setTool(tool.label as ViewerTool)}
                className={classes.mainLink}
                data-active={tool.label === activeTool || undefined}
            >
                <tool.icon size={22} stroke={1.5} />
            </UnstyledButton>
        </Tooltip>
    ));

    return (
        <nav className={classes.navbar}>
            <div className={classes.wrapper}>
                {/* Left icon strip */}
                <div className={classes.aside}>
                    <div className={classes.logo}>
                        <img src={novaLogo} width={45} alt="Nova logo" />
                    </div>
                    {toolIcons}
                </div>

                {/* Right settings panel */}
                <div className={classes.main}>
                    <Title order={5} className={classes.title}>
                        {activeTool}
                    </Title>

                    {activeTool === 'Zoom' && <ZoomToolSettings />}
                    {activeTool === 'Windowing' && <CTWindowingToolSettings/>}
                    {activeTool === 'Measure' && <MeasurementList />}
                    {activeTool === 'Pan' && <p>Pan is active. Drag to move image.</p>}
                    {activeTool === 'Rotate' && <p>Rotate the image using gestures.</p>}
                    {activeTool === 'None' && <p>Select a tool to get started.</p>}
                </div>
            </div>
        </nav>
    );
}
