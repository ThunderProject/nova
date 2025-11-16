// components/CTWindowingToolSettings/PresetSelect.tsx
import {
    Combobox,
    Box,
    Group,
    Text,
    ActionIcon,
    useCombobox,
} from '@mantine/core';
import { IconX } from '@tabler/icons-react';
import { useEffect } from 'react';
import styles from './PresetSelect.module.css';

type PresetSelectProps = {
    presets: Record<string, { width: number; level: number }>;
    activePreset: string | null;
    onSelect: (name: string | null) => void;
    onDelete: (name: string) => void;
};

export function PresetSelect({ activePreset, onDelete, onSelect, presets, }: PresetSelectProps) {
    const combobox = useCombobox();

    useEffect(() => {
        if (!Object.keys(presets).length) {
            combobox.closeDropdown();
        }
    }, [combobox, presets]);

    return (
        <Combobox
            store={combobox}
            onOptionSubmit={(value) => {
                onSelect(value);
                combobox.closeDropdown();
            }}
            withinPortal>
            <Combobox.Target>
                <Box
                    onClick={() => combobox.toggleDropdown()}
                    tabIndex={0}
                    className={styles.selectInput}
                >
                    <Text size="sm" style={{ color: 'white' }}>
                        {activePreset || 'Select preset'}
                    </Text>
                    <Combobox.Chevron />
                </Box>
            </Combobox.Target>

            <Combobox.Dropdown className={styles.dropdown}>
                <Combobox.Options className={styles.options}>
                    {Object.keys(presets).map((name) => (
                        <Combobox.Option key={name} value={name} className={styles.optionRow}>
                            <Group justify="space-between" w="100%">
                                <Text size="sm">{name}</Text>
                                <ActionIcon
                                    variant="subtle"
                                    color="gray"
                                    size="xs"
                                    className={styles.deleteButton}
                                    onClick={(e) => {
                                        e.stopPropagation();
                                        onDelete(name);
                                        combobox.closeDropdown();
                                    }}
                                >
                                    <IconX size={14} stroke={1.25} />
                                </ActionIcon>
                            </Group>
                        </Combobox.Option>
                    ))}
                </Combobox.Options>
            </Combobox.Dropdown>
        </Combobox>
    );
}
