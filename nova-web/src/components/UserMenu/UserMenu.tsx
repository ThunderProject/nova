import {
    Menu,
    Avatar,
    Text,
    Divider,
    UnstyledButton, Group,
} from "@mantine/core";
import { IconUserCircle, IconLogout } from "@tabler/icons-react";
import { useNavigate } from "react-router-dom";
import { useAuthStore } from "../../stores/AuthStore.ts";
import { NovaApi } from "../../nova_api/NovaApi.ts";

export function UserMenu() {
    const navigate = useNavigate();
    const { logout, username } = useAuthStore();

    const handleLogout = async () => {
        if (await NovaApi.Logout()) {
            logout();
            navigate("/login", { replace: true });
        }
    };

    return (
        <Group ml="auto">
            <Menu shadow="md" position="bottom-end" width={200}>
                <Menu.Target>
                    <UnstyledButton style={{ marginRight: 8 }}>
                        <Avatar radius="xl" size={32}>
                            <IconUserCircle size={22} />
                        </Avatar>
                    </UnstyledButton>
                </Menu.Target>

                <Menu.Dropdown>
                    <Text size="sm" px="md" py="xs">
                        Signed in as
                    </Text>

                    <Text size="md" px="md" fw={500}>
                        {username}
                    </Text>

                    <Divider my="sm" />

                    <Menu.Item
                        leftSection={<IconLogout size={16} />}
                        color="red"
                        onClick={handleLogout}
                    >
                        Logout
                    </Menu.Item>
                </Menu.Dropdown>
            </Menu>
        </Group>
    );
}
