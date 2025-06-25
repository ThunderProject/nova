import {
    ActionIcon,
    Tooltip,
    Modal,
    Button,
    Group,
    TextInput,
    Text,
    Divider,
    Stack,
} from '@mantine/core';
import {IconAlertTriangle, IconFolder, IconPlus, IconX} from '@tabler/icons-react';
import {useEffect, useRef, useState} from 'react';
import { logger } from '../lib/Logger.ts';
import { open } from '@tauri-apps/plugin-dialog'
import {FileSystem} from "../lib/FileSystem.ts";
import styles from './CreateProjectButton.module.css';
import toast from "react-hot-toast";

interface OpenProjectButtonProps {
    iconSize?: number;
    onClicked: () => void;
    onClosed: () => void;
    onFileSelected: (filePath: string) => void;
    modalOpen: boolean;
    setModalOpen: (open: boolean) => void;
}

const supportedFileExtensions: string[] = ['zip', 'dcm'];

export function CreateProjectButton({
                                        iconSize = 24,
                                        onClosed,
                                        onClicked,
                                        onFileSelected,
                                        modalOpen,
                                        setModalOpen
                                    }: OpenProjectButtonProps) {

    const [baseFolder, setBaseFolder] = useState('');
    const [projectName, setProjectName] = useState('');
    const [selectedFile, setSelectedFile] = useState<File | null>(null);
    const [folderNotEmpty, setFolderNotEmpty] = useState(false);
    const folderSeparator = baseFolder.includes('\\') ? '\\' : '/';
    const modalRef = useRef<HTMLDivElement>(null);
    const [shake, setShake] = useState(false);

    const fullProjectPath = (() => {
        if (!baseFolder) {
            return '';
        }
        if (!projectName) {
            return baseFolder;
        }
        return `${baseFolder.replace(/[\\/]+$/, '')}${folderSeparator}${projectName.trim()}`;
    })();

    const handleChooseDirectory = async () => {
        try {
            const selectedPath = await open({
                title: "Select folder",
                multiple: false,
                directory: true,
            });

            if (typeof selectedPath === 'string') {
                logger.debug(`selected folder: ${selectedPath}`)
                setBaseFolder(selectedPath);

                const isFolderEmpty = await FileSystem.isEmpty(selectedPath);
                setFolderNotEmpty(!isFolderEmpty);

                if(!isFolderEmpty) {
                    logger.warn("Selected folder is not empty. Contents may be overwritten");
                }
            }
        }
        catch (error) {
            logger.error(`Failed t select folder: ${error}`);
        }
    };

    const handleCreate = () => {
        setModalOpen(false);
        logger.debug(`Creating project at: ${fullProjectPath}`);
        if (selectedFile) logger.debug(`Selected file: ${selectedFile.name}`);
        onFileSelected(fullProjectPath);
    };

    useEffect(() => {
        const handleClickOutside = (event: MouseEvent) => {
            if (modalRef.current && !modalRef.current.contains(event.target as Node)) {
                setShake(true);
                setTimeout(() => setShake(false), 300);
            }
        };

        if (modalOpen) {
            document.addEventListener("mousedown", handleClickOutside);
        }

        return () => {
            document.removeEventListener("mousedown", handleClickOutside);
        };
    }, [modalOpen]);

    return (
        <>
            <Tooltip label="Create project">
                <ActionIcon
                    variant="subtle"
                    size="lg"
                    color="gray"
                    onClick={() => {
                        setModalOpen(true);
                        setTimeout(onClicked, 0);
                    }}
                >
                    <IconPlus size={iconSize} />
                </ActionIcon>
            </Tooltip>

            <Modal
                opened={modalOpen}
                onClose={() => {
                    setModalOpen(false);
                    onClosed();
                }}
                title="Create New Project"
                centered
                size="xl"
                closeOnClickOutside={false}
                overlayProps={{
                    backgroundOpacity: 0.55,
                    blur: 3,
                }}
                closeButtonProps={{
                    icon: <IconX size={20} stroke={1.5}/>,
                }}
                classNames={{
                    close: styles.customClose,
                }}
                radius="md"
            >

                <Stack gap="sm" mb="md">
                    <div>
                        <Group align="center" gap="sm">
                            <TextInput
                                label="Project name"
                                size="sm"
                                required
                                withAsterisk
                                radius="md"
                                value={projectName}
                                onChange={(e) => setProjectName(e.target.value)}
                                style={{flex: 1}}
                            />
                        </Group>
                    </div>

                    <div>
                        <Group align="center" gap="sm">
                            <TextInput
                                label="Location"
                                size="sm"
                                radius="md"
                                required
                                value={fullProjectPath}
                                readOnly
                                placeholder="Select a folder..."
                                rightSection={
                                    <Tooltip label="Browse...">
                                        <ActionIcon
                                            onClick={handleChooseDirectory}
                                            variant="subtle"
                                            color="gray"
                                            radius="xl"
                                        >
                                            <IconFolder size={20}/>
                                        </ActionIcon>
                                    </Tooltip>
                                }
                                style={{flex: 1}}
                            />
                        </Group>
                        {folderNotEmpty && (
                            <Group gap={4} align="center" mt={4}>
                                <IconAlertTriangle size={16} color="var(--mantine-color-yellow-6)"/>
                                <Text size="xs" c="yellow">
                                    Folder is not empty. Files may be overwritten.
                                </Text>
                            </Group>
                        )}
                    </div>

                    <div>
                        <Group>
                            <TextInput
                                label="Import .zip or .dcm file:"
                                size="sm"
                                radius="md"
                                placeholder="No file selected"
                                value={selectedFile?.name || ''}
                                readOnly
                                rightSection={
                                    <Tooltip label="Browse...">
                                        <ActionIcon
                                            onClick={handleChooseDirectory}
                                            variant="subtle"
                                            color="gray"
                                            radius="xl"
                                        >
                                            <IconFolder size={20}/>
                                        </ActionIcon>
                                    </Tooltip>
                                }
                                style={{flex: 1}}
                            />
                        </Group>
                    </div>
                </Stack>

                <Divider my="sm"/>

                <Group>
                    <Button variant="default" onClick={() => {
                        setModalOpen(false);
                        onClosed();
                    }}>
                        Cancel
                    </Button>
                    <Button onClick={handleCreate} disabled={!fullProjectPath}>
                        Create
                    </Button>
                </Group>
            </Modal>
        </>
    );
}
