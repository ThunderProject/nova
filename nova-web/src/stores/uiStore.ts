import { create } from 'zustand';

interface UIState {
    sidebarWidth: number;
    setSidebarWidth: (width: number) => void;
}

export const useUIStore = create<UIState>((set) => ({
    setSidebarWidth: (width) => set({ sidebarWidth: width }),
    sidebarWidth: 300,
}));