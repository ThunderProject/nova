import { CloseButton, Group, Paper, Text } from '@mantine/core';
import { IconLayoutSidebarLeftCollapse } from '@tabler/icons-react';

export default function ViewportPanel({ onClose }: { onClose: () => void }) {
    return (
        <Paper
            shadow="md"
            radius="sm"
            style={{
                height: 600,
                width: 800,
                display: 'flex',
                flexDirection: 'column',
                backgroundColor: 'var(--mantine-color-dark-)',
            }}
        >
            {/* "Tab" header */}
            <Group gap="xs" align="center">
                <IconLayoutSidebarLeftCollapse size={16} />
                <Text size="sm" fw={500}>2D View</Text>
                <CloseButton onClick={onClose} size="sm" />
            </Group>


            {/* Viewport content */}
            <div style={{flex: 1 }}>
                {}
                <div style={{
                    width: '100%',
                    height: '100%',
                    backgroundColor: 'black',
                    border: '1px solid var(--mantine-color-dark-7)',
                }}>
                    {/* canvas goes here later */}
                </div>
            </div>
        </Paper>
    );
}
