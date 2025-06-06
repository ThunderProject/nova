import {
    Box,
    Button,
    Group,
    NumberInput,
    Stack,
    Text,
    Title,
    TextInput,
    Collapse, Slider, rem,
    Paper, Tooltip, Checkbox,
} from '@mantine/core';
import {
    IconPlus,
    IconInfoCircle,
    IconAlertTriangle,
} from '@tabler/icons-react';
import {useEffect, useState} from 'react';
import { useViewerToolsStore } from '../../stores/ViewerToolStore';
import styles from './CTWindowingToolSettings.module.css'
import toast from "react-hot-toast";
import {CTWindowingPresetsLoader} from "../../utils/CTWindowingPresetsLoader.ts";
import {logger} from "../../lib/Logger.ts";
import {modals} from "@mantine/modals";
import {PresetSelect} from "./PresetSelect.tsx";

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
    const { windowSettings, setWindowSettings, addPreset, userPresets, setUserPresets, activePreset, setActivePreset } = useViewerToolsStore();
    const [showAddPreset, setShowAddPreset] = useState(false);

    const [presetName, setPresetName] = useState('');
    const [presetWidth, setPresetWidth] = useState<number | ''>(defaultPresetWindowWidth);
    const [presetLevel, setPresetLevel] = useState<number | ''>(defaultPresetWindowLevel);
    const [nameError, setNameError] = useState(false);
    const [duplicateWarning, setDuplicateWarning] = useState(false)

    useEffect(() => {
        const load = async () => {
            if(Object.keys(userPresets).length !== 0) {
                return;
            }

            const presets = await CTWindowingPresetsLoader.load();
            if(!presets) {
                logger.error("Failed to load presets");
                return;
            }
            setUserPresets(presets)
        }
        void load();
    }, [setUserPresets, userPresets])

    const handleSelect = (preset: string | null) => {
        if (preset && preset in userPresets) {
            setWindowSettings(userPresets[preset]);
            setActivePreset(preset);
        }
        else {
            setActivePreset(null)
        }
    };

    const handleAdd = () => {
        if (!presetName || presetWidth === '' || presetLevel === '') {
            logger.error("Failed to add entry preset. Reason: one or more values are invalid")
            return;
        }

        logger.info(`Adding ${presetName} to presets`)

        addPreset({
            name: presetName,
            width: Number(presetWidth),
            level: Number(presetLevel),
        });

        successToast('Preset added successfully');

        setPresetName('');
        setPresetWidth(presetWidth);
        setPresetLevel(presetLevel);
        setShowAddPreset(false);
        setDuplicateWarning(false);
        setNameError(false);
        setWindowSettings(userPresets[presetName]);
        setActivePreset(presetName);
    };

    const handleDelete = (name: string) => {
        modals.openConfirmModal( {
            title: 'Delete Preset',
            centered: true,
            children: (
                <Text size="sm">
                    Are you sure you want to delete the <strong>{name}</strong> preset? This action cannot be undone.
                </Text>
            ),
            labels: { confirm: 'Delete', cancel: 'Cancel' },
            confirmProps: { color: 'red' },
            onConfirm: () => {
                const updatedPresets = { ...userPresets };
                delete updatedPresets[name];
                setUserPresets(updatedPresets);

                if (activePreset === name) {
                    setActivePreset(null);
                }

                successToast(`Deleted preset "${name}"`);
            },
        });
    };

    return (
        <Stack gap="lg" mt="md">
            <Box px="md">
                <Title order={6} mb="xs">Preset</Title>

                <PresetSelect
                    presets={userPresets}
                    activePreset={activePreset}
                    onSelect={handleSelect}
                    onDelete={handleDelete}
                />
            </Box>

            <Box px="md">
                <Group justify="space-between" align="center" mb="xs">
                    <Title order={6} mb={0}>
                        Window Width
                    </Title>
                    <NumberInput
                        value={windowSettings.width}
                        onChange={(val) => {
                            if (typeof val === 'number') {
                                setWindowSettings({ width: val });
                            }
                        }}
                        min={1}
                        max={4000}
                        hideControls
                        classNames={{ input: styles.inlineInput }}
                    />
                </Group>

                <Slider
                    value={windowSettings.width}
                    onChange={(val) => setWindowSettings({ width: val })}
                    min={1}
                    max={4000}
                    step={1}
                    color="blue"
                    styles={{
                        thumb: { width: rem(16), height: rem(16), border: '2px solid white' },
                        track: { height: rem(4) },
                    }}
                />
            </Box>

            <Box px="md">
                <Group justify="space-between" align="center" mb="xs">
                    <Title order={6} mb={0}>
                        Window Level
                    </Title>
                    <NumberInput
                        value={windowSettings.level}
                        onChange={(val) => {
                            if (typeof val === 'number') {
                                setWindowSettings({ level: val });
                            }
                        }}
                        min={-1000}
                        max={1000}
                        hideControls
                        classNames={{ input: styles.inlineInput }}
                    />
                </Group>
                <Slider
                    value={windowSettings.level}
                    onChange={(val) => setWindowSettings({ level: val })}
                    min={-1000}
                    max={1000}
                    step={1}
                    color="teal"
                    style={{ flex: 1 }}
                    styles={{
                        thumb: { width: rem(16), height: rem(16), border: '2px solid white' },
                        track: { height: rem(4) },
                    }}
                />
            </Box>

            <Box px="md" mt="md">
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
                                    const isDuplicate = Object.keys(userPresets)
                                        .map(n => n.toLowerCase())
                                        .includes(value.toLowerCase());

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
                                disabled={!nameError && presetName !== ''}
                            >
                                <Button
                                    fullWidth
                                    onClick={handleAdd}
                                    radius="md"
                                    variant="outline"
                                    size="md"
                                    disabled={nameError || presetName === ''}
                                    className={styles.addButton}
                                    styles={{
                                        root: {
                                            borderColor: 'var(--mantine-color-blue-6)',
                                            backgroundColor: nameError || presetName === '' ? 'transparent' : 'inherit',
                                            color: 'var(--mantine-color-blue-2)',
                                            transition: 'all 150ms ease',
                                            opacity: nameError || presetName === '' ? 0.4 : 1,
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
