import {Tabs, ActionIcon, Tooltip, rem} from '@mantine/core';
import {
    IconFileArrowRight,
    IconFolderOpen,
    IconDeviceFloppy,
    IconMenu2
} from '@tabler/icons-react';
// import { MantineLogo } from '@mantinex/mantine-logo';
import classes from './MainTabBar.module.css';
import {useEffect, useRef, useState} from "react";
import nova_logo from '../../assets/nova_icon.png';

const tabs = [
    'Patient data',
    'Viewer',
    'Export',
];

export function MainTabBar() {
    const iconSize = 24;
    const logoSize = 24;
    const [menuOpen, setMenuOpen] = useState(false);
    const menuRef = useRef<HTMLDivElement>(null);

    useEffect(() => {
        const handleClickOutside = (e: MouseEvent) => {
            if (menuRef.current && !menuRef.current.contains(e.target as Node)) {
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
    }, [menuOpen]);

    return (
        <div className={classes.header}>
            {/* Toolbar Row */}
            <div className={classes.toolbarRow}>
                <img src={nova_logo} alt="Nova logo" style={{width: logoSize, height: logoSize, marginLeft: 4, marginRight: 8}}/>
                {menuOpen ? (
                    <div ref={menuRef} style={{display: 'inline-flex', gap: rem(8)}}>
                        <Tooltip label="Open project">
                            <ActionIcon
                                variant="subtle"
                                size="lg"
                                color="gray"
                                onClick={() => {
                                    setMenuOpen(false);
                                }}
                            >
                                <IconFolderOpen size={iconSize}/>
                            </ActionIcon>
                        </Tooltip>

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
                        defaultValue="Patient data"
                        variant="outline"
                        classNames={{
                            list: classes.tabsList,
                            tab: classes.tab,
                        }}
                    >
                        <Tabs.List>
                            {tabs.map((tab) => (
                                <Tabs.Tab key={tab} value={tab}>
                                    {tab}
                                </Tabs.Tab>
                            ))}
                        </Tabs.List>
                    </Tabs>
                </div>
            </div>
        </div>
    );
}
