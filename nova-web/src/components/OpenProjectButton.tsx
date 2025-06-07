import { ActionIcon, Tooltip } from '@mantine/core';
import { IconFolderOpen } from '@tabler/icons-react'
import { open } from '@tauri-apps/plugin-dialog'
import {logger} from "../lib/Logger.ts";

interface OpenProjectButtonProps {
    iconSize?: number;
    onClicked: () => void;
    onFileSelected: (filePath: string) => void;
}

const supportedFileExtensions: string[] = ['zip', 'dcm'];

export function OpenProjectButton({ iconSize = 24, onClicked, onFileSelected }: OpenProjectButtonProps) {
    const handleClick = async () => {
        try {
            onClicked();
            const selectedPath = await open({
                title: "Open project",
                filters: [{ name: 'Project Files', extensions: supportedFileExtensions }],
                multiple: false,
                directory: false,
            });

            if (typeof selectedPath === 'string') {
                logger.debug(`selected file: ${selectedPath}`);
                onFileSelected(selectedPath);
            }
        }
        catch (error) {
            logger.error(`Failed to open project: ${error}`);
        }
    };

    return (
        <Tooltip label="Open project">
            <ActionIcon variant="subtle" size="lg" color="gray" onClick={handleClick}>
                <IconFolderOpen size={iconSize} />
            </ActionIcon>
        </Tooltip>
    );
}