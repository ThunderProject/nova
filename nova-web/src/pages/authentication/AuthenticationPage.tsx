import {
    ActionIcon,
    Anchor,
    Button,
    Checkbox,
    Group,
    Paper,
    PasswordInput,
    Text,
    TextInput,
    Title,
    Tooltip,
} from "@mantine/core";
import { useState } from "react";
import { IconRefresh } from "@tabler/icons-react";
import { PasswordGenerator } from "../../utils/PasswordGenerator";
import classes from "./AuthenticationPage.module.css";

export function AuthenticationPage() {
    const [password, setPassword] = useState("");

    const handleGenerate = () => {
        const newPassword = PasswordGenerator.generate();
        setPassword(newPassword);
    };

    return (
        <div className={classes.wrapper}>
            <Paper className={classes.form}>
                <Title order={2} className={classes.title}>
                    Welcome back to Nova!
                </Title>

                <TextInput placeholder="Username" size="md" radius="md" />

                <PasswordInput
                    placeholder="Password"
                    mt="md"
                    size="md"
                    radius="md"
                    value={password}
                    onChange={(e) => setPassword(e.currentTarget.value)}
                />

                <Group justify="space-between" align="center" mt="md">
                    <Checkbox label="Keep me logged in" size="md" />
                    <Tooltip label="Generate a secure password">
                        <ActionIcon
                            variant="light"
                            color="blue"
                            size="lg"
                            radius="md"
                            onClick={handleGenerate}
                        >
                            <IconRefresh size={18} />
                        </ActionIcon>
                    </Tooltip>
                </Group>

                <Button fullWidth mt="xl" size="md" radius="md">
                    Login
                </Button>

                <Text ta="center" mt="md">
                    Don&apos;t have an account?{" "}
                    <Anchor href="#" fw={500} onClick={(event) => event.preventDefault()}>
                        Register
                    </Anchor>
                </Text>
            </Paper>
        </div>
    );
}
