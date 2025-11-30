import {
    Group,
    Stack,
} from "@mantine/core";
import { useEffect, useRef, useState } from "react";
import {ProjectToolbar} from "../ProjectToolbar/ProjectToolbar.tsx";
import { UserMenu } from "../UserMenu/UserMenu.tsx";
import {NavigationTabs} from "../NavigationTabs/NavigationTabs.tsx";
import classes from "./MainTabBar.module.css";


export function MainTabBar() {
    const [createProjectModalOpen, setCreateProjectModalOpen] = useState(false);
    const [menuOpen, setMenuOpen] = useState(false);
    const menuRef = useRef<HTMLDivElement>(null);

    useEffect(() => {
        const handleClickOutside = (e: MouseEvent) => {
            if (!createProjectModalOpen && menuRef.current && !menuRef.current.contains(e.target as Node)) {
                setMenuOpen(false);
            }
        };

        if (menuOpen) {
            document.addEventListener("mousedown", handleClickOutside);
        }
        else {
            document.removeEventListener("mousedown", handleClickOutside);
        }

        return () => {
            document.removeEventListener("mousedown", handleClickOutside);
        };
    }, [createProjectModalOpen, menuOpen]);


    return (
        <Stack gap={0} className={classes.header}>
            <Group className={classes.toolbarRow} align="center" justify="space-between">
                <ProjectToolbar
                    iconSize={24}
                    menuOpen={menuOpen}
                    createProjectModalOpen={createProjectModalOpen}
                    setMenuOpen={setMenuOpen}
                    setCreateProjectModalOpen={setCreateProjectModalOpen}
                    menuRef={menuRef}
                />

                <NavigationTabs />
                <UserMenu />
            </Group>
        </Stack>
    );
}
