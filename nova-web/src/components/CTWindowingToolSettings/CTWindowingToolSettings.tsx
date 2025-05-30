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
    Paper,
} from '@mantine/core';
import { IconPlus } from '@tabler/icons-react';
import { useState } from 'react';
import { useViewerToolsStore } from '../../stores/ViewerToolStore';
import styles from './CTWindowingToolSettings.module.css'

type PresetName = 'Brain' | 'Lung' | 'Bone' | 'SoftTissue';

const presets: Record<PresetName, { width: number; level: number }> = {
    Brain: { width: 80, level: 40 },
    Lung: { width: 1500, level: -600 },
    Bone: { width: 2000, level: 300 },
    SoftTissue: { width: 400, level: 40 },
};

export default function CTWindowingToolSettings() {
    const { windowSettings, setWindowSettings, addPreset } = useViewerToolsStore();
    const [showAddPreset, setShowAddPreset] = useState(false);

    const [presetName, setPresetName] = useState('');
    const [presetWidth, setPresetWidth] = useState<number | ''>('');
    const [presetLevel, setPresetLevel] = useState<number | ''>('');
    const [nameError, setNameError] = useState(false);


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

        setPresetName('');
        setPresetWidth('');
        setPresetLevel('');
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
                            <TextInput
                                label="Name"
                                placeholder="e.g. Tumor Boost"
                                value={presetName}
                                onChange={(e) => {
                                    setPresetName(e.currentTarget.value);
                                    setNameError(false);
                                }}
                                error={nameError && 'Name is required'}
                                required
                            />
                            <NumberInput
                                label="Window Width"
                                placeholder="400"
                                value={presetWidth}
                                min={1}
                                max={4000}
                                onChange={(val) => setPresetWidth(val)}
                            />
                            <NumberInput
                                label="Window Level"
                                placeholder="40"
                                value={presetLevel}
                                min={-1000}
                                max={1000}
                                onChange={(val) => setPresetLevel(val)}
                            />
                            <Button
                                onClick={handleAdd}
                                fullWidth
                            >
                                Add
                            </Button>
                        </Stack>
                    </Paper>
                </Collapse>
            </Box>
        </Stack>
    );
}
