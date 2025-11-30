import {
    Tabs,
    ActionIcon,
    Tooltip,
    rem,
    Group,
    Menu,
    UnstyledButton,
    Avatar,
    Text,
    Divider,
    Stack,
} from "@mantine/core";
import {IconDeviceFloppy, IconFileArrowRight, IconLogout, IconMenu2, IconUserCircle} from "@tabler/icons-react";
import { useEffect, useRef, useState } from "react";
import { useNavigate, useLocation } from "react-router-dom";
import nova_logo from "../../assets/nova_icon.png";
import { OpenProjectButton } from "../OpenProjectButton.tsx";
import { Project } from "../../project/project.ts";
import { CreateProjectButton } from "../CreateProjectButton/CreateProjectButton.tsx";
import {NovaApi} from "../../nova_api/NovaApi.ts";
import {useAuthStore} from "../../stores/AuthStore.ts";
import classes from "./MainTabBar.module.css";

const tabs = [
    { label: "Patient data", path: "/patients" },
    { label: "Viewer", path: "/viewer" },
    { label: "Export", path: "/export" },
];

export function MainTabBar() {
    const iconSize = 24;
    const logoSize = 24;
    const navigate = useNavigate();
    const location = useLocation();
    const [createProjectModalOpen, setCreateProjectModalOpen] = useState(false);
    const [menuOpen, setMenuOpen] = useState(false);
    const menuRef = useRef<HTMLDivElement>(null);
    const { logout, username }  = useAuthStore();

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
        <Stack gap={0} className={classes.header}>

            <Group className={classes.toolbarRow} align="center" justify="space-between">
                <Group style={{ minWidth: rem(210) }}>
                    <img src={nova_logo} alt="Nova logo" style={{ height: logoSize, marginLeft: 4, width: logoSize}} />
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
                </Group>

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

                <Group ml="auto">
                    <Menu shadow="md" position="bottom-end" width={200}>
                        <Menu.Target>
                            <UnstyledButton style={{ marginRight: 8 }}>
                                <Avatar radius="xl" size={32}>
                                    <IconUserCircle size={22} />
                                </Avatar>
                            </UnstyledButton>
                        </Menu.Target>

                        <Menu.Dropdown>
                            <Text size="sm" px="md" py="xs">
                                Signed in as
                            </Text>

                            <Text size="md" px="md" fw={500}>
                                {username}
                            </Text>

                            <Divider my="sm" />

                            <Menu.Item
                                leftSection={<IconLogout size={16} />}
                                color="red"
                                onClick={async () => {
                                    if(await NovaApi.Logout()) {
                                        logout();
                                        logout();
                                        navigate("/login", { replace: true });
                                    }
                                }}
                            >
                                Logout
                            </Menu.Item>
                        </Menu.Dropdown>
                    </Menu>
                </Group>
            </Group>
        </Stack>
    );
}
