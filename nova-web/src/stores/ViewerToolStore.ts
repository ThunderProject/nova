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
    activeTool: 'None',
    setTool: (tool) => set({ activeTool: tool }),

    windowSettings: { width: 400, level: 40 },
    setWindowSettings: (settings) =>
        set((state) => ({
            windowSettings: { ...state.windowSettings, ...settings },
        })),

    userPresets: {},
    setUserPresets: (data) => set({ userPresets: data }),

    activePreset: null,
    setActivePreset: (name) => set({ activePreset: name }),

    addPreset: (preset) =>
        set((state) => ({
            userPresets: {
                ...state.userPresets,
                [preset.name]: {
                    width: preset.width,
                    level: preset.level,
                },
            },
        })),
}));
