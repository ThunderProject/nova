import { getCurrentWindow  } from "@tauri-apps/api/window";
import "./TopBar.module.css";

export default function TopBar() {
    const win = getCurrentWindow();

    return (
        <div className="titlebar" data-tauri-drag-region>
            <div className="title">Nova</div>
            <div className="controls" style={{ WebkitAppRegion: "no-drag" }}>
                <button title="Minimize" onClick={() => win?.minimize()}>
                    <svg width="14" height="14" viewBox="0 0 24 24">
                        <path fill="currentColor" d="M19 13H5v-2h14z" />
                    </svg>
                </button>
                <button title="Maximize" onClick={() => win?.toggleMaximize()}>
                    <svg width="14" height="14" viewBox="0 0 24 24">
                        <path fill="currentColor" d="M4 4h16v16H4zm2 4v10h12V8z" />
                    </svg>
                </button>
                <button className="close" title="Close" onClick={() => win?.close()}>
                    <svg width="14" height="14" viewBox="0 0 24 24">
                        <path
                            fill="currentColor"
                            d="M13.46 12L19 17.54V19h-1.46L12 13.46L6.46 19H5v-1.46L10.54 12L5 6.46V5h1.46L12 10.54L17.54 5H19v1.46z"
                        />
                    </svg>
                </button>
            </div>
        </div>
    );
}
