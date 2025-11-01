import { describe, it, expect, beforeEach } from "vitest";
import {GridManager, type GridConfig} from "./GridManager.ts";

describe("GridManager", () => {
    let gridManager: GridManager;
    const cfg: GridConfig = { columns: 2, rows: 2 };

    beforeEach(() => {
        gridManager = new GridManager(cfg);
    });

    it("starts with one empty tab and active index 0", () => {
        expect(gridManager.groups()).toEqual([[]]);
        expect(gridManager.getActiveTab()).toBe(0);
    });

    it("adds viewports until cap, then creates new tab", () => {
        const ids = ["v1", "v2", "v3", "v4", "v5"];

        for (const id of ids) {
            gridManager.add(id);
        }

        const groups = gridManager.groups();
        expect(groups.length).toBe(2);
        expect(groups[0]).toEqual(["v1", "v2", "v3", "v4"]);
        expect(groups[1]).toEqual(["v5"]);
    });

    it("does not duplicate when adding same id twice", () => {
        gridManager.add("v1");
        gridManager.add("v1");
        const groups = gridManager.groups();
        expect(groups).toEqual([["v1"]]);
    });

    it("removes viewport from tab", () => {
        const ids = ["v1", "v2", "v3"];
        ids.forEach((id) => gridManager.add(id));

        gridManager.remove("v2");

        expect(gridManager.groups()[0]).toEqual(["v1", "v3"]);
        expect(gridManager.indexOf("v2")).toBeNull();
    });

    it("removes empty tab", () => {
        ["v1", "v2", "v3", "v4", "v5"].forEach((id) => gridManager.add(id));

        const result = gridManager.remove("v5");
        expect(result.tabRemoved).toBe(1);
        const groups = gridManager.groups();

        expect(groups.length).toBe(1);
        expect(groups[0]).toEqual(["v1", "v2", "v3", "v4"]);
    });

    it("clamps active tab when removing a higher-index tab", () => {
        ["v1", "v2", "v3", "v4", "v5", "v6"].forEach((id) => gridManager.add(id));

        gridManager.setActiveTab(1);
        expect(gridManager.getActiveTab()).toBe(1);

        gridManager.remove("v5");
        expect(gridManager.getActiveTab()).toBe(0);
    });

    it("ignores invalid active tab indices", () => {
        gridManager.setActiveTab(-1);
        expect(gridManager.getActiveTab()).toBe(0);
        gridManager.setActiveTab(100);
        expect(gridManager.getActiveTab()).toBe(0);
    });

    it("resets correctly", () => {
        ["v1", "v2"].forEach((id) => gridManager.add(id));
        gridManager.reset();
        expect(gridManager.groups()).toEqual([[]]);
        expect(gridManager.getActiveTab()).toBe(0);
    });

    it("returns correct ViewportPosition", () => {
        ["a", "b", "c"].forEach((id) => gridManager.add(id));
        const viewportPos = gridManager.indexOf("b");
        expect(viewportPos).not.toBeNull();
        expect(viewportPos?.tabIndex).toBe(0);
        expect(typeof viewportPos?.indexInTab).toBe("number");
    });
});
