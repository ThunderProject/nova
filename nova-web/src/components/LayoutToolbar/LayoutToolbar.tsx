import { Group, Button, Menu } from "@mantine/core";
import { IconPlus } from "@tabler/icons-react";
import LayoutSwitcher, {type LayoutOption} from "../LayoutSwitcher/LayoutSwitcher.tsx";

type LayoutToolbarProps<T extends string> = {
    layout: T;
    onChangeLayout: (layout: T) => void;
    layoutOptions: LayoutOption<T>[];
    actionLabel?: string;
    actions?: { label: string; onClick: () => void }[];
    disabled?: boolean;
    size?: "xs" | "sm" | "md";
};

export default function LayoutToolbar<T extends string>({
                                                            actionLabel = "Actions",
                                                            actions = [],
                                                            disabled = false,
                                                            layout,
                                                            layoutOptions,
                                                            onChangeLayout,
                                                            size = "xs",
                                                        }: LayoutToolbarProps<T>) {
    const hasActions = actions.length > 0;

    return (
        <Group justify="space-between" mb="xs">
            <Group gap="xs">
                <LayoutSwitcher<T>
                    value={layout}
                    onChange={onChangeLayout}
                    options={layoutOptions}
                    disabled={disabled}
                    size={size}
                />

                {hasActions && (
                    <Menu shadow="md" width={170}>
                        <Menu.Target>
                            <Button size={size} leftSection={<IconPlus size={14} />}>
                                {actionLabel}
                            </Button>
                        </Menu.Target>
                        <Menu.Dropdown>
                            {actions.map((action) => (
                                <Menu.Item key={action.label} onClick={action.onClick}>
                                    {action.label}
                                </Menu.Item>
                            ))}
                        </Menu.Dropdown>
                    </Menu>
                )}
            </Group>
        </Group>
    );
}
