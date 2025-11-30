import { ActionIcon, Tooltip, rem, Group } from "@mantine/core";
import { IconDeviceFloppy, IconFileArrowRight, IconMenu2 } from "@tabler/icons-react";
import React from "react";
import nova_logo from "../../assets/nova_icon.png";
import { OpenProjectButton } from "../OpenProjectButton.tsx";
import { CreateProjectButton } from "../CreateProjectButton/CreateProjectButton.tsx";
import { Project } from "../../project/project.ts";

interface ProjectToolbarProps {
    iconSize: number;
    menuOpen: boolean;
    menuRef: React.RefObject<HTMLDivElement | null>;
    createProjectModalOpen: boolean;
    setMenuOpen: (open: boolean) => void;
    setCreateProjectModalOpen: (open: boolean) => void;
}

export function ProjectToolbar({
                                   iconSize,
                                   menuOpen,
                                   menuRef,
                                   createProjectModalOpen,
                                   setMenuOpen,
                                   setCreateProjectModalOpen,
                               }: ProjectToolbarProps) {
    const logoSize = 24;

    return (
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
    );
}
