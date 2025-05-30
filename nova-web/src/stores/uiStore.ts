import { create } from 'zustand';

interface UIState {
    sidebarWidth: number;
    setSidebarWidth: (width: number) => void;
}

export const useUIStore = create<UIState>((set) => ({
    sidebarWidth: 300,
    setSidebarWidth: (width) => set({ sidebarWidth: width }),
}));