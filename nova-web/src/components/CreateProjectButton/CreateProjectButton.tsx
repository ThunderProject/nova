import {
    ActionIcon,
    Tooltip,
    Modal,
    Button,
    Group,
    TextInput,
    Text,
    Divider,
    Stack, Loader,
} from '@mantine/core';
import {IconAlertTriangle, IconFolder, IconPlus, IconX} from '@tabler/icons-react';
import {useEffect, useState} from 'react';
import { open } from '@tauri-apps/plugin-dialog'
import {modals} from "@mantine/modals";
import { logger } from '../../lib/Logger.ts';
import {FileSystem} from "../../lib/FileSystem.ts";
import {Project} from "../../project/project.ts";
import styles from './CreateProjectButton.module.css';

interface OpenProjectButtonProps {
    iconSize?: number;
    onClicked: () => void;
    onClosed: () => void;
    modalOpen: boolean;
    setModalOpen: (open: boolean) => void;
}

const supportedFileExtensions: string[] = ['zip', 'dcm'];

export function CreateProjectButton({
                                        iconSize = 24,
                                        onClosed,
                                        onClicked,
                                        modalOpen,
                                        setModalOpen
                                    }: OpenProjectButtonProps) {

    const [baseFolder, setBaseFolder] = useState('');
    const [projectName, setProjectName] = useState('');
    const [selectedFiles, setSelectedFiles] = useState<string[]>([]);
    const [selectedFilesInput, setSelectedFilesInput] = useState('');
    const [selectedFilesError, setSelectedFilesError] = useState<string | null>(null);
    const [folderNotEmpty, setFolderNotEmpty] = useState(false);
    const folderSeparator = baseFolder.includes('\\') ? '\\' : '/';
    const [shake, setShake] = useState(false);
    const [isCreating, setIsCreating] = useState(false);

    const invalidProjectNameCharacters = ['/', '\\', ':', '*', '?', '"', '<', '>', '|'];
    const [projectNameError, setProjectNameError] = useState<string | null>(null);

    const fullProjectPath = (() => {
        if (!baseFolder) {
            return '';
        }
        if (!projectName) {
            return baseFolder;
        }
        return `${baseFolder.replace(/[\\/]+$/, '')}${folderSeparator}${projectName.trim()}`;
    })();

    const handleProjectNameChange = (value: string) => {
        const invalidChar = value.split('').find(char => invalidProjectNameCharacters.includes(char));

        if(invalidChar) {
            setProjectNameError(`Project name contains an invalid character: "${invalidChar}"`)
        }
        else {
            setProjectNameError(null);
        }
        setProjectName(value);
    }

    const handleChooseDirectory = async () => {
        try {
            const selectedPath = await open({
                title: "Select folder",
                multiple: false,
                directory: true,
            });

            if (typeof selectedPath === 'string') {
                const projectPath = await FileSystem.join([selectedPath, projectName]);
                logger.debug(`selected folder: ${projectPath}`)
                setBaseFolder(selectedPath);

                // If the combined path (selected folder + project name) doesn’t exist yet,
                // there’s no need to check if it’s empty. The directory will be created later
                // when the project is generated, so we can safely return early here.
                if(!await FileSystem.exist(projectPath)) {
                    return;
                }

                const isFolderEmpty = await FileSystem.isEmpty(projectPath);
                setFolderNotEmpty(!isFolderEmpty);

                if(!isFolderEmpty) {
                    logger.warn("Selected folder is not empty. Contents may be overwritten or deleted");
                }
            }
            else if(selectedPath === null) {
                //User cancelled the selection
                setFolderNotEmpty(false);
                setBaseFolder('');
            }
            else {
                //Not sure if wee can end up here, but let's be robust and handle it.
                logger.warn("Selected folder path has an incorrect type. Expected: string")
                setBaseFolder('');
                setFolderNotEmpty(false);
            }
        }
        catch (error) {
            logger.error(`Failed to select folder: ${error}`);
            setBaseFolder('');
        }
    };

    const handleImportFiles = async () => {
        try {
            const files = await open({
                directory: false,
                filters: [{ extensions: supportedFileExtensions, name: 'Files' }],
                multiple: true,
                title: "Import files",
            });

            if(files === null) {
                setSelectedFiles([]);
                setSelectedFilesInput('');
                return;
            }

            const filesArray = Array.isArray(files) ? files : [files];
            setSelectedFiles(filesArray);
            setSelectedFilesInput(filesArray.join('; '));
        }
        catch (error) {
            logger.error(`Failed to import files: ${error}`);
        }
    }

    const handleFilesInputChange = (value: string) => {
        setSelectedFilesInput(value);
        const parsed = value
            .split(';')
            .map(f => f.trim())
            .filter(f => f.length > 0);
        setSelectedFiles(parsed);
    }

    const handleCreate = async () => {
        try {
            if(!fullProjectPath) {
                return;
            }

            if (selectedFiles !== null) {
                logger.debug(`Selected files: ${selectedFiles}`);

                const invalidFiles = selectedFiles.filter(file => {
                    const ext = file.split('.').pop()?.toLowerCase();
                    return !supportedFileExtensions.includes(ext || '');
                })

                const checkFileExist = await Promise.all(
                    selectedFiles.map((file) => FileSystem.exist(file))
                );

                const selectedFilesExist = checkFileExist.every(Boolean);

                if(!selectedFilesExist || invalidFiles.length > 0) {
                    const errorMessage = 'One or more selected files do not exist, or have an unsupported file format (only .zip and .dcm are supported)';
                    setSelectedFilesError(errorMessage);
                    setShake(true);
                    setTimeout(() => setShake(false), 300);
                    logger.error(errorMessage);
                    return;
                }
                else {
                    setSelectedFilesError(null);
                }
            }

            if(folderNotEmpty) {
                modals.openConfirmModal({
                    centered: true,
                    children: (
                        <Text size="sm">
                            The selected folder is not empty. Some files may be overwritten or deleted.
                            Are you sure you want to continue?
                        </Text>
                    ),
                    confirmProps: { color: 'blue' },
                    labels: { cancel: 'Cancel', confirm: 'Yes, continue' },
                    onConfirm: async () => {
                        await createProject();
                    },
                    title: 'Folder Not Empty',
                })
            }
            else {
                await createProject();
            }
        }
        catch (error) {
            logger.error(`Failed to create project. Reason: ${error}`);
        }
    };

    const createProject = async () => {
        setIsCreating(true);

        //for now simulate a delay
        await new Promise((r) => setTimeout(r, 2000));

        logger.debug(`Creating project at: ${fullProjectPath}`);

        await Project.createNewProject({
            importedFiles: selectedFiles,
            projectName: projectName,
            workingDirectory: baseFolder
        })

        setIsCreating(false);
        setModalOpen(false);
        onClosed();
    }

    useEffect(() => {
        const handleClickOutside = (event: MouseEvent) => {
            const modalInner = document.querySelector('.mantine-Modal-inner') as HTMLElement | null;
            const modalHeader = document.querySelector('.mantine-Modal-header') as HTMLElement | null;

            const target = event.target as Node;

            if (modalInner && !modalInner.contains(target) && !(modalHeader && modalHeader.contains(target))) {
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
                    content: shake ? styles.shake : undefined,
                }}
                radius="md"
            >
                {isCreating && (
                    <div className={styles.overlay}>
                        <div className={styles.spinnerContainer}>
                            <Loader size="lg" />
                            <Text size="md" mt="xs">
                                Creating project <b>{projectName}</b>...
                            </Text>
                        </div>
                    </div>
                )}
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
                                onChange={(e) => handleProjectNameChange(e.currentTarget.value)}
                                style={{flex: 1}}
                            />
                        </Group>
                        {projectNameError && (
                            <Group gap={4} align="center" mt={4}>
                                <IconX  size={16} color="var(--mantine-color-red-6)" />
                                <Text size="xs" c="red">
                                    {projectNameError}
                                </Text>
                            </Group>
                        )}
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
                                    Folder is not empty. Files may be overwritten or deleted.
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
                                value={selectedFilesInput}
                                onChange={(e) => handleFilesInputChange(e.currentTarget.value)}
                                rightSection={
                                    <Tooltip label="Browse...">
                                        <ActionIcon
                                            onClick={handleImportFiles}
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
                        {selectedFilesError && (
                            <Group gap={4} align="center" mt={4}>
                                <IconX  size={16} color="var(--mantine-color-red-6)" />
                                <Text size="xs" c="red">
                                    {selectedFilesError}
                                </Text>
                            </Group>
                        )}
                    </div>
                </Stack>

                <Divider my="sm"/>

                <Group>
                    <Button
                        size="md"
                        variant="default"
                        onClick={() => {
                        setModalOpen(false);
                        onClosed();
                    }}>
                        Cancel
                    </Button>
                    <Button
                        onClick={handleCreate}
                        disabled={!fullProjectPath || !projectName || projectNameError !== null}
                        className={styles.createButton}
                        size="md"
                    >
                        Create
                    </Button>
                </Group>

            </Modal>
        </>
    );
}
