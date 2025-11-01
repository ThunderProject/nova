import { create } from "zustand";
import { subscribeWithSelector } from "zustand/middleware";

export type Capability =
    | { kind: "windowing"; min: number; max: number; defaultCenter: number; defaultWidth: number }

export type DisplayableId = string;
export type ViewportId = string;

export type WindowingParams = { center: number; width: number };

export type LayerParams = | { kind: "windowing"; value: WindowingParams }

export type ViewportLayer = {
    id: string,
    displayableId: DisplayableId,
    rendererKind: "CT",
    params: LayerParams;
};

export type ViewportModel = {
    id: ViewportId;
    title: string;
    visible: boolean;
    layers: ViewportLayer[];
    activeLayerId?: string;
};

export type Displayable = {
    id: DisplayableId;
    modality: "CT"
    capabilities: Capability[];
};

type ViewerStore = {
    layout: "tabs" | "grid";
    viewports: Record<ViewportId, ViewportModel>;
    tabViewports: Record<string, ViewportId[]>;
    order: ViewportId[];

    addViewportToTab: (tabId: string, viewport: ViewportModel) => void;
    removeViewportFromTab: (tabId: string, viewportId: ViewportId) => void;
    selectedViewportId?: ViewportId;
    setSelectedViewportId: (id: ViewportId) => void;
    setLayout: (k: ViewerStore["layout"]) => void;
    addViewport: (viewport: Omit<ViewportModel, "visible"> & { visible: boolean }) => ViewportId;
    removeViewport: (id: ViewportId) => void;
    setViewportVisible: (id: ViewportId, visible: boolean) => void;
    setLayerParams: (viewportId: ViewportId, layerId: string, params: LayerParams) => void;
};

export const useViewerStore = create(subscribeWithSelector<ViewerStore>((set, get) => ({
    addViewport: (viewport) => {
        const id = viewport.id;

        set((s) => ({
            order: [...s.order, id],
            viewports: { ...s.viewports, [id]: { ...viewport, visible: viewport.visible ?? true } },
        }));
        return id;
    },
    addViewportToTab: (tabId, viewport) =>
        set((state) => {
            const vp = { ...viewport, visible: viewport.visible ?? true };
            const tab = state.tabViewports[tabId] ?? [];
            return {
                tabViewports: { ...state.tabViewports, [tabId]: [...tab, vp.id] },
                viewports: { ...state.viewports, [vp.id]: vp },
            };
        }),
    layout: "tabs",
    order: [],
    removeViewport: (id) => set((store) => {
        const { [id]: _, ...rest } = store.viewports;
        return { order: store.order.filter(x => x !== id), viewports: rest };
    }),

    removeViewportFromTab: (tabId, viewportId) =>
        set((state) => {
            const newTabs = { ...state.tabViewports };
            newTabs[tabId] = (newTabs[tabId] ?? []).filter((id) => id !== viewportId);

            const stillUsed = Object.values(newTabs).some((ids) => ids.includes(viewportId));
            const newViewports = { ...state.viewports };

            if (!stillUsed) {
                delete newViewports[viewportId];
            }

            return { tabViewports: newTabs, viewports: newViewports };
        }),
    selectedViewportId: undefined,
    setLayerParams: (viewportId, layerId, params) =>
        set((s) => {
            const vp = s.viewports[viewportId];
            if (!vp) {
                return s;
            }

            const layers = vp.layers.map(l => l.id === layerId ? { ...l, params } : l);
            return { viewports: { ...s.viewports, [viewportId]: { ...vp, layers } } };
        }),
    setLayout: (layout) => set({layout}),
    setSelectedViewportId: (id) => set({ selectedViewportId: id }),
    setViewportVisible: (id, visible) => set((s) => ({
        viewports: { ...s.viewports, [id]: { ...s.viewports[id], visible } },
    })),
    tabViewports: {},
    viewports: {},
})));

type DisplayablesStore = {
    displayables: Record<DisplayableId, Displayable>;
    register: (d: Displayable) => void;
    getById: (id: DisplayableId) => Displayable | undefined;
};

export const useDisplayables = create<DisplayablesStore>((set, get) => ({
    displayables: {},
    getById: (id) => get().displayables[id],
    register: (d) => set((s) => ({ displayables: { ...s.displayables, [d.id]: d } })),
}));