export type ViewportId = string;

export interface GridConfig {
    columns: number;
    rows: number;
}

export interface ViewportPosition  {
    tabIndex: number;
    indexInTab: number;
}

export class GridManager {
    #tabs: ViewportId[][] = [[]];
    #activeTabIndex = 0;
    readonly #cap: number;

    constructor(config: GridConfig) {
        this.#cap = config.columns * config.rows;
    }

    public groups(): ViewportId[][] {
        return this.#tabs.map((tab) => [...tab]);
    }

    public getActiveTab(): number {
        return this.#activeTabIndex;
    }

    public setActiveTab(index: number): void {
        if(index < 0 || index >= this.#tabs.length) {
            return
        }
        this.#activeTabIndex = index;
    }

    public add(id: ViewportId): ViewportPosition  {
        const existing = this.indexOf(id);
        if(existing) {
            return existing;
        }

        const activeTab = this.#tabs[this.#activeTabIndex];
        const lastTabIndex = this.#tabs.length - 1;
        const lastTab = this.#tabs[lastTabIndex];

        let targetTab: number;

        if(activeTab.length < this.#cap ) {
            targetTab = this.#activeTabIndex;
        }
        else if(lastTab.length < this.#cap) {
            targetTab = lastTabIndex;
        }
        else {
            targetTab = this.#tabs.length;
            this.#tabs.push([]);
        }

        this.#tabs[targetTab].push(id);

        return {
            indexInTab: this.#tabs[targetTab].length - 1,
            tabIndex: targetTab
        };
    }

    public remove(id: ViewportId): { removedFrom?: ViewportPosition ; tabRemoved?: number } {
        const existing = this.indexOf(id);
        if(!existing) {
            return {};
        }

        const { indexInTab, tabIndex } = existing;
        const tab = this.#tabs[tabIndex];
        tab.splice(indexInTab, 1);

        let tabRemoved: number | undefined;

        if(tab.length === 0 && this.#tabs.length > 1) {
            this.#tabs.splice(tabIndex, 1);
            tabRemoved = tabIndex;

            if(this.#activeTabIndex >= this.#tabs.length) {
                this.#activeTabIndex = this.#tabs.length - 1;
            }
            else if(tabIndex <= this.#activeTabIndex && this.#activeTabIndex > 0) {
                this.#activeTabIndex -= 1;
            }
        }

        return {
            removedFrom: existing,
            tabRemoved
        }
    }

    public indexOf(id: ViewportId): ViewportPosition  | null {
        const existingTabIndex = this.#tabs.findIndex(tab => tab.includes(id));

        if(existingTabIndex !== -1) {
            const indexInTab = this.#tabs[existingTabIndex].indexOf(id);
            return { indexInTab, tabIndex: existingTabIndex };
        }

        return null;
    }

    public reset(): void {
        this.#tabs = [[]];
        this.#activeTabIndex = 0;
    }
}