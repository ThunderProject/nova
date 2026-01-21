import {
    Alert,
    Anchor,
    Button, Checkbox,
    Group,
    Loader,
    Paper,
    PasswordInput,
    Text,
    TextInput,
    Title,
} from "@mantine/core";
import { useState } from "react";
import { useNavigate } from "react-router-dom";
import { NovaApi } from "../../nova_api/NovaApi.ts";
import classes from "./SignupPage.module.css";
import { useAuthStore } from "../../stores/AuthStore.ts";

export function SignupPage() {
    const [username, setUsername] = useState("");
    const [password, setPassword] = useState("");
    const [confirmPassword, setConfirmPassword] = useState("");
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

    const handleSignup = async () => {
        setError(null);

        if(!username) {
            setError("Username cannot be empty");
            return;
        }

        if(!password) {
            setError("Password cannot be empty");
            return;
        }

        if (password !== confirmPassword) {
            setError("Passwords do not match");
            return;
        }

        setLoading(true);
        const result = await NovaApi.SignUp(username, password)
        setLoading(false);

        if (result.hasError()) {
            setError(result.error);
            return;
        }

        await handleLogin();
    };

    return (
        <div className={classes.centerWrapper}>
            <Paper className={classes.glassForm}>
                <Title order={2} className={classes.title}>
                    Create your Nova account
                </Title>

                <Text className={classes.subtitle}>
                    Sign up to start your workspace
                </Text>

                <TextInput
                    placeholder="Username"
                    size="md"
                    radius="md"
                    value={username}
                    onChange={(e) => setUsername(e.currentTarget.value)}
                />

                <PasswordInput
                    placeholder="Password"
                    mt="md"
                    size="md"
                    radius="md"
                    value={password}
                    onChange={(e) => setPassword(e.currentTarget.value)}
                />

                <PasswordInput
                    placeholder="Confirm password"
                    mt="md"
                    size="md"
                    radius="md"
                    value={confirmPassword}
                    onChange={(e) => setConfirmPassword(e.currentTarget.value)}
                />

                {error && (
                    <Alert
                        title="Signup failed"
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
                    className={classes.signupButton}
                    onClick={handleSignup}
                    disabled={loading}
                >
                    {loading ? (
                        <Group gap="xs">
                            <Loader size="sm" />
                            <span>Signing up...</span>
                        </Group>
                    ) : (
                        "Sign up"
                    )}
                </Button>

                <Text ta="center" mt="md">
                    Already have an account?{" "}
                    <Anchor
                        href="#"
                        fw={500}
                        onClick={(event) => {
                            navigate("/login")
                            event.preventDefault()
                        }}
                    >
                        Login
                    </Anchor>
                </Text>
            </Paper>
        </div>
    );
}
