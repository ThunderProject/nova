import { Group, SegmentedControl, Text } from "@mantine/core";
import type { ReactNode } from "react";

export type LayoutOption<T extends string = string> = {
    value: T;
    label: string;
    icon?: ReactNode;
};

type LayoutSwitcherProps<T extends string = string> = {
    value: T;
    onChange: (value: T) => void;
    options: LayoutOption<T>[];
    disabled?: boolean;
    size?: "xs" | "sm" | "md" | "lg";
};

export default function LayoutSwitcher<T extends string>({
                                                             disabled = false,
                                                             onChange,
                                                             options,
                                                             size = "xs",
                                                             value,
                                                         }: LayoutSwitcherProps<T>) {
    const data = options.map(({ icon, label, value }) => ({
        label: (
            <Group gap={4} align="center" justify="center" wrap="nowrap">
                {icon}
                <Text size="xs" style={{ lineHeight: 1 }}>
                    {label}
                </Text>
            </Group>
        ),
        value,
    }));

    return (
        <SegmentedControl
            size={size}
            value={value}
            onChange={(v) => onChange(v as T)}
            data={data}
            disabled={disabled}
        />
    );
}
