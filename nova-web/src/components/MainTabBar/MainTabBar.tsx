import {Tabs, ActionIcon, Tooltip, rem} from '@mantine/core';
import {
    IconFileArrowRight,
    IconDeviceFloppy,
    IconMenu2
} from '@tabler/icons-react';
import classes from './MainTabBar.module.css';
import {useEffect, useRef, useState} from "react";
import nova_logo from '../../assets/nova_icon.png';
import { useNavigate, useLocation } from 'react-router-dom';
import {OpenProjectButton} from "../OpenProjectButton.tsx";
import {Project} from "../../project/project.ts";
import {CreateProjectButton} from "../CreateProjectButton.tsx";

const tabs = [
    { label: 'Patient data', path: '/patients' },
    { label: 'Viewer', path: '/viewer' },
    { label: 'Export', path: '/export' },
];

export function MainTabBar() {
    const iconSize = 24;
    const logoSize = 24;
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
            document.addEventListener('mousedown', handleClickOutside);
        }
        else {
            document.removeEventListener('mousedown', handleClickOutside);
        }

        return () => {
            document.removeEventListener('mousedown', handleClickOutside);
        };
    }, [createProjectModalOpen, menuOpen]);

    const navigate = useNavigate();
    const location = useLocation();

    const currentTab = tabs.find((tab) => location.pathname.startsWith(tab.path))?.label || 'Patient data';
    const handleTabChange = (label: string | null) => {
        if(!label) {
            return;
        }

        const tab = tabs.find((tab) => tab.label === label);
        if(tab) {
            navigate(tab.path);
        }
    }

    return (
        <div className={classes.header}>
            {/* Toolbar Row */}
            <div className={classes.toolbarRow}>
                <img src={nova_logo} alt="Nova logo" style={{width: logoSize, height: logoSize, marginLeft: 4, marginRight: 8}}/>
                {menuOpen ? (
                    <div ref={menuRef} style={{display: 'inline-flex', gap: rem(8)}}>
                        <CreateProjectButton
                            iconSize={iconSize}
                            onClicked={() => {}}
                            onClosed={() => setMenuOpen(false) }
                            modalOpen={createProjectModalOpen}
                            setModalOpen={setCreateProjectModalOpen}
                        >
                        </CreateProjectButton>

                        <OpenProjectButton
                            iconSize={iconSize}
                            onClicked={ () => setMenuOpen(false) }
                            onFileSelected={async (file) => {
                                await Project.open(file)
                            }}
                        >
                        </OpenProjectButton>

                        <Tooltip label="Save project">
                            <ActionIcon
                                variant="subtle"
                                size="lg"
                                color="gray"
                                onClick={() => {
                                    setMenuOpen(false);
                                }}
                            >
                                <IconDeviceFloppy size={iconSize}/>
                            </ActionIcon>
                        </Tooltip>

                        <Tooltip label="Export project">
                            <ActionIcon
                                variant="subtle"
                                size="lg"
                                color="gray"
                                onClick={() => {
                                    setMenuOpen(false);
                                }}
                            >
                                <IconFileArrowRight size={iconSize}/>
                            </ActionIcon>
                        </Tooltip>
                    </div>

                ) : (
                    <ActionIcon
                        variant="subtle"
                        size="lg"
                        color="gray"
                        onClick={() => setMenuOpen(true)}
                    >
                        <IconMenu2 size={iconSize}/>
                    </ActionIcon>
                )}
            </div>

            {/* Tabs */}
            <div className={classes.tabsRow}>
                <div className={classes.tabsIndent}>
                    <Tabs
                        value={currentTab}
                        onChange={handleTabChange}
                        variant="outline"
                        classNames={{
                            list: classes.tabsList,
                            tab: classes.tab,
                        }}
                    >
                        <Tabs.List>
                            {tabs.map((tab) => (
                                <Tabs.Tab key={tab.label} value={tab.label}>
                                    {tab.label}
                                </Tabs.Tab>
                            ))}
                        </Tabs.List>
                    </Tabs>
                </div>
            </div>
        </div>
    );
}
