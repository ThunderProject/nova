import {
    Alert,
    Anchor,
    Button,
    Checkbox,
    Group, Loader,
    Paper,
    PasswordInput,
    Text,
    TextInput,
    Title,
} from "@mantine/core";
import { useState } from "react";
import { useNavigate } from "react-router-dom";
import {NovaApi} from "../../nova_api/NovaApi.ts";
import classes from "./AuthenticationPage.module.css";
import {useAuthStore} from "../../stores/AuthStore.ts";

export function AuthenticationPage() {
    const [username, seUsername] = useState("");
    const [password, setPassword] = useState("");
    const [keepUserLoggedIn, setKeepUserLoggedIn] = useState(false);
    const [error, setError] = useState<string | null>(null);
    const [loading, setLoading] = useState(false);
    const navigate = useNavigate();
    const { login } = useAuthStore();

    const handleLogin = async () => {
        setError(null);
        setLoading(true);

        const loginResult = await NovaApi.login(username, password, keepUserLoggedIn);

        setLoading(false);

        if(loginResult.hasError()) {
            setError(loginResult.error)
            return;
        }

        login(username);
        navigate("/viewer")
    }

    return (
        <div className={classes.wrapper}>
            <Paper className={classes.form}>
                <Title order={2} className={classes.title}>
                    Welcome back to Nova!
                </Title>
                <Text className={classes.subtitle}>
                    Sign in to access your workspace
                </Text>

                <TextInput
                    placeholder="Username"
                    size="md"
                    radius="md"
                    onChange={(e) => seUsername(e.currentTarget.value)}/>

                <PasswordInput
                    placeholder="Password"
                    mt="md"
                    size="md"
                    radius="md"
                    value={password}
                    onChange={(e) => setPassword(e.currentTarget.value)}
                />

                {error && (
                    <Alert
                        title="Login failed"
                        color="red"
                        radius="sm"
                        mt="sm"
                        variant="light"
                    >
                        {error}
                    </Alert>
                )}

                <Group justify="space-between" align="center" mt="md">
                    <Checkbox
                        label="Keep me logged in"
                        size="md"
                        checked={keepUserLoggedIn}
                        onChange={(e) => setKeepUserLoggedIn(e.currentTarget.checked)}
                    />
                </Group>

                <Button
                    fullWidth
                    mt="xl"
                    size="md"
                    radius="md"
                    className={classes.loginButton}
                    onClick={handleLogin}
                    disabled={loading}
                >
                    {loading ? (
                        <Group gap="xs">
                            <Loader size="sm" />
                            <span>Logging in...</span>
                        </Group>
                    ) : (
                        "Login"
                    )}
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
