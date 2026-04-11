import { create } from "zustand";
import {RenderContext} from "../renderer/RenderContext";

type RenderState = {
    ctx: RenderContext | null;
    initRenderer: (canvas: HTMLCanvasElement) => Promise<void>;
    getCtx: () => RenderContext | null;
};


export const useRenderStore = create<RenderState>((set, get) => ({
    ctx: null,
    getCtx: () => get().ctx,
    initRenderer: async (canvas) => {
        let ctx = get().ctx;
        if (!ctx) {
            ctx = new RenderContext();
            await ctx.init(canvas);
            set({ ctx });
        }
    },
}));