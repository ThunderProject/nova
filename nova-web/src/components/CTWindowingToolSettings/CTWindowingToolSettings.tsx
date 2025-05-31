import {
    Box,
    Button,
    Group,
    NumberInput,
    Stack,
    Text,
    Title,
    TextInput,
    Collapse, Slider, rem, Select,
    Paper, Tooltip, Checkbox,
} from '@mantine/core';
import {
    IconPlus,
    IconInfoCircle,
    IconAlertTriangle
} from '@tabler/icons-react';
import { useState } from 'react';
import { useViewerToolsStore } from '../../stores/ViewerToolStore';
import styles from './CTWindowingToolSettings.module.css'
import toast from "react-hot-toast";

type PresetName = 'Brain' | 'Lung' | 'Bone' | 'SoftTissue';

const presets: Record<PresetName, { width: number; level: number }> = {
    Brain: { width: 80, level: 40 },
    Lung: { width: 1500, level: -600 },
    Bone: { width: 2000, level: 300 },
    SoftTissue: { width: 400, level: 40 },
};

const successToast = (message: string) =>
    toast.success(message, {
        style: {
            background: 'var(--mantine-color-dark-6)',
            border: '1px solid var(--mantine-color-green-5)',
            color: 'white',
            padding: '6px 6px',
            borderRadius: 'var(--mantine-radius-md)',
        },
        iconTheme: {
            primary: 'var(--mantine-color-green-5)',
            secondary: 'var(--mantine-color-dark-6)',
        },
    });

const defaultPresetWindowWidth = 400;
const defaultPresetWindowLevel = 40;

export default function CTWindowingToolSettings() {
    const { windowSettings, setWindowSettings, addPreset } = useViewerToolsStore();
    const [showAddPreset, setShowAddPreset] = useState(false);

    const [presetName, setPresetName] = useState('');
    const [presetWidth, setPresetWidth] = useState<number | ''>(defaultPresetWindowWidth);
    const [presetLevel, setPresetLevel] = useState<number | ''>(defaultPresetWindowLevel);
    const [nameError, setNameError] = useState(false);
    const [duplicateWarning, setDuplicateWarning] = useState(false);
    const existingPresetNames = Object.keys(presets).map(name => name.toLowerCase());

    const handlePresetChange = (preset: string | null) => {
        if (preset && preset in presets) {
            setWindowSettings(presets[preset as PresetName]);
        }
    };

    const handleAdd = () => {
        if (!presetName || presetWidth === '' || presetLevel === '') {
            return;
        }

        addPreset({
            name: presetName,
            width: Number(presetWidth),
            level: Number(presetLevel),
        });

        successToast('Preset added successfully');

        setPresetName('');
        setPresetWidth(defaultPresetWindowWidth);
        setPresetLevel(defaultPresetWindowLevel);
        setShowAddPreset(false);
    };

    return (
        <Stack gap="lg" mt="md">
            <Box px="md">
                <Title order={6} mb="xs">Preset</Title>
                <Select
                    data={Object.keys(presets)}
                    placeholder="Select preset"
                    onChange={handlePresetChange}
                    value={
                        (Object.entries(presets).find(
                            ([, v]) =>
                                v.width === windowSettings.width &&
                                v.level === windowSettings.level
                        )?.[0] as PresetName) || null
                    }
                    radius="md"
                    withCheckIcon={false}
                    classNames={{
                        input: styles.selectInput, // 👈 targets input wrapper
                    }}
                    styles={{
                        input: {
                            transition: 'all 150ms ease',
                            '&:hover': {
                                borderColor: 'var(--mantine-color-blue-6)',
                                backgroundColor: 'var(--mantine-color-dark-5)',
                            },
                        },
                    }}
                />
            </Box>

            <Box px="md">
                <Group justify="space-between" mb="xs">
                    <Title order={6}>Window Width</Title>
                    <Text size="xs" c="dimmed">{windowSettings.width}</Text>
                </Group>
                <Slider
                    min={1}
                    max={4000}
                    value={windowSettings.width}
                    onChange={(val) => setWindowSettings({ width: val })}
                    color="blue"
                    styles={{
                        thumb: { width: rem(16), height: rem(16), border: '2px solid white' },
                        track: { height: rem(4) },
                    }}
                />
            </Box>

            <Box px="md">
                <Group justify="space-between" mb="xs">
                    <Title order={6}>Window Level</Title>
                    <Text size="xs" c="dimmed">{windowSettings.level}</Text>
                </Group>
                <Slider
                    min={-1000}
                    max={1000}
                    value={windowSettings.level}
                    onChange={(val) => setWindowSettings({ level: val })}
                    color="teal"
                    styles={{
                        thumb: { width: rem(16), height: rem(16), border: '2px solid white' },
                        track: { height: rem(4) },
                    }}
                />
            </Box>
            <Box px="md">
                <Group justify="space-between" mb="xs">
                    <Button
                        variant="light"
                        size="xs"
                        fullWidth={true}
                        leftSection={<IconPlus size={14} />}
                        onClick={() => setShowAddPreset((v) => !v)}
                    >
                        {showAddPreset ? 'Cancel' : 'Add Preset'}
                    </Button>
                </Group>

                <Collapse in={showAddPreset}>
                    <Paper withBorder shadow="sm" radius="md" p="md">
                        <Stack gap="sm">
                            <Checkbox
                                // checked={preserve}
                                // onChange={(e) => setPreserve(e.currentTarget.checked)}
                                label={
                                    <Group gap={4} wrap="nowrap">
                                        <Text size="sm">Persist</Text>
                                        <Tooltip
                                            label="Save this preset permanently for future launches"
                                            withArrow
                                            position="right"
                                        >
                                            <Text span style={{ cursor: 'help', color: 'var(--mantine-color-blue-4)' }}>
                                                <IconInfoCircle
                                                    size={16}
                                                    style={{ cursor: 'help' }}
                                                    stroke={1.5}
                                                />
                                            </Text>
                                        </Tooltip>
                                    </Group>
                                }
                            />
                            <TextInput
                                label="Name"
                                placeholder="e.g. Liver"
                                value={presetName}
                                onChange={(e) => {
                                    const value = e.currentTarget.value;
                                    const isEmpty = (str: string) => (!str?.trim().length);
                                    const isDuplicate = existingPresetNames.includes(value.toLowerCase());

                                    const empty = isEmpty(value)

                                    setPresetName(value);
                                    setNameError(empty);

                                    setPresetName(value);
                                    setNameError(empty);
                                    setDuplicateWarning(!empty && isDuplicate);
                                }}
                                description={
                                    duplicateWarning && (
                                        <Group gap={4} align="center">
                                            <IconAlertTriangle size={16} color="var(--mantine-color-yellow-6)" />
                                            A preset with this name already exists. It will be overwritten.
                                        </Group>
                                    )
                                }
                                error={nameError && 'Name is required'}
                                required
                                styles={{
                                    input: {
                                        borderColor: duplicateWarning
                                            ? 'var(--mantine-color-yellow-6)'
                                            : undefined,
                                    },
                                }}
                            />
                            <NumberInput
                                label="Window Width"
                                value={presetWidth}
                                min={1}
                                max={4000}
                                onChange={(val) => {
                                    if(val === '' || typeof val === 'number') {
                                        setPresetWidth(val);
                                    }
                                }}
                            />
                            <NumberInput
                                label="Window Level"
                                value={presetLevel}
                                min={-1000}
                                max={1000}
                                onChange={(val) => {
                                    if(val === '' || typeof val === 'number') {
                                        setPresetLevel(val);
                                    }
                                }}
                            />
                            <Tooltip
                                label="Please enter a name first"
                                position="bottom"
                                withArrow
                                disabled={!nameError} // Only show when nameError is true
                            >
                                <Button
                                    fullWidth
                                    onClick={handleAdd}
                                    radius="md"
                                    variant="outline"
                                    size="md"
                                    disabled={nameError}
                                    className={styles.addButton}
                                    styles={{
                                        root: {
                                            borderColor: 'var(--mantine-color-blue-6)',
                                            backgroundColor: nameError ? 'transparent' : 'inherit',
                                            color: 'var(--mantine-color-blue-2)',
                                            transition: 'all 150ms ease',
                                            opacity: nameError ? 0.4 : 1,
                                        },
                                        label: {
                                            fontWeight: 600,
                                            letterSpacing: 0.25,
                                        },
                                    }}
                                >
                                    Add
                                </Button>
                            </Tooltip>
                        </Stack>
                    </Paper>
                </Collapse>
            </Box>
        </Stack>
    );
}
