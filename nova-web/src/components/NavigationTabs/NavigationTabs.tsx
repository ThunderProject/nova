import {Group, Tabs} from "@mantine/core";
import { useLocation, useNavigate } from "react-router-dom";
import classes from "./NavigationTabs.module.css";

const tabs = [
    { label: "Patient data", path: "/patients" },
    { label: "Viewer", path: "/viewer" },
    { label: "Export", path: "/export" },
];

export function NavigationTabs() {
    const location = useLocation();
    const navigate = useNavigate();

    const currentTab = tabs.find((t) => location.pathname.startsWith(t.path))?.label ?? null;

    const handleTabChange = (label: string | null) => {
        if (!label) {
            return;
        }

        const tab = tabs.find((t) => t.label === label);
        if (tab) {
            navigate(tab.path);
        }
    }

    return (
        <Group justify="center" style={{ flex: 1 }}>
            <Tabs
                value={currentTab}
                onChange={handleTabChange}
                variant="outline"
                classNames={{ list: classes.tabsList, tab: classes.tab }}
            >
                <Tabs.List>
                    {tabs.map((tab) => (
                        <Tabs.Tab key={tab.label} value={tab.label}>
                            {tab.label}
                        </Tabs.Tab>
                    ))}
                </Tabs.List>
            </Tabs>
        </Group>
    );
}
