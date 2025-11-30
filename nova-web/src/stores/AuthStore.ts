import { create } from "zustand";

type AuthState = {
    username: string | null;
    isAuthenticated: boolean;

    login: (username: string) => void;
    logout: () => void;
    setAuth: (isAuth: boolean, username?: string | null) => void;
}

export const useAuthStore = create<AuthState>((set) => ({
    isAuthenticated: false,
    username: null,

    login: (username) => set({ isAuthenticated: true, username }),
    logout: () => set({ isAuthenticated: false, username: null }),
    setAuth: (isAuth, username = null) => set({ isAuthenticated: isAuth, username }),
}));