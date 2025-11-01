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

    userPresets: Record<string, { width: number; level: number }>;
    setUserPresets: (data: Record<string, { width: number; level: number }>) => void;

    activePreset: string | null;
    setActivePreset: (name: string | null) => void;

    addPreset: (preset: Preset) => void;
}

export const useViewerToolsStore = create<ViewerToolsState>((set) => ({
    activePreset: null,
    activeTool: 'None',

    addPreset: (preset) =>
        set((state) => ({
            userPresets: {
                ...state.userPresets,
                [preset.name]: {
                    level: preset.level,
                    width: preset.width,
                },
            },
        })),
    setActivePreset: (name) => set({ activePreset: name }),

    setTool: (tool) => set({ activeTool: tool }),
    setUserPresets: (data) => set({ userPresets: data }),

    setWindowSettings: (settings) =>
        set((state) => ({
            windowSettings: { ...state.windowSettings, ...settings },
        })),
    userPresets: {},

    windowSettings: { level: 40, width: 400 },
}));
