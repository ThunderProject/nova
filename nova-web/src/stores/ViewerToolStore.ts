import { create } from 'zustand';

export type ViewerTool = 'Zoom' | 'Pan' | 'Windowing' | 'Rotate' | 'Measure' | 'None';

export interface WindowLevelSettings {
    width: number;
    level: number;
}

export interface Preset {
    name: string;
    width: number;
    level: number;
}

interface ViewerToolsState {
    activeTool: ViewerTool;
    setTool: (tool: ViewerTool) => void;

    windowSettings: WindowLevelSettings;
    setWindowSettings: (settings: Partial<WindowLevelSettings>) => void;

    customPresets: Preset[];
    addPreset: (preset: Preset) => void;
}

export const useViewerToolsStore = create<ViewerToolsState>((set) => ({
    activeTool: 'None',
    setTool: (tool) => set({ activeTool: tool }),

    windowSettings: { width: 400, level: 40 },
    setWindowSettings: (settings) =>
        set((state) => ({
            windowSettings: { ...state.windowSettings, ...settings },
        })),

    customPresets: [],
    addPreset: (preset) =>
        set((state) => ({
            customPresets: [...state.customPresets, preset],
        })),
}));
