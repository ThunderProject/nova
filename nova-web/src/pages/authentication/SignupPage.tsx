import {
    Alert,
    Anchor,
    Button, Center, Group,
    Loader,
    Paper,
    PasswordInput,
    Text,
    TextInput,
    Title, Transition,
} from "@mantine/core";
import {useEffect, useRef, useState} from "react";
import { useNavigate } from "react-router-dom";
import { NovaApi } from "../../nova_api/NovaApi.ts";
import classes from "./SignupPage.module.css";

export function SignupPage() {
    const [username, setUsername] = useState("");
    const [password, setPassword] = useState("");
    const [confirmPassword, setConfirmPassword] = useState("");
    const [error, setError] = useState<string | null>(null);
    const [success, setSuccess] = useState(false);
    const [countdown, setCountdown] = useState<number>(3);
    const [loading, setLoading] = useState(false);

    const navigate = useNavigate();

    const redirectTimeoutRef = useRef<number | null>(null);
    const countdownIntervalRef = useRef<number | null>(null);

    const clearRedirectTimers = () => {
        if (redirectTimeoutRef.current !== null) {
            window.clearTimeout(redirectTimeoutRef.current);
            redirectTimeoutRef.current = null;
        }
        if (countdownIntervalRef.current !== null) {
            window.clearInterval(countdownIntervalRef.current);
            countdownIntervalRef.current = null;
        }
    };

    useEffect(() => {
        return () => clearRedirectTimers();
    }, []);

    const navigateToLogin = () => {
        clearRedirectTimers();
        navigate("/login");
    };

    const startRedirectCountdown = () => {
        clearRedirectTimers();
        setCountdown(3);

        countdownIntervalRef.current = window.setInterval(() => {
            setCountdown((countdown) => (countdown > 1 ? countdown - 1 : 1));
        }, 1000);

        redirectTimeoutRef.current = window.setTimeout(() => {
            navigateToLogin();
        }, 3000);
    };

    const handleSignup = async () => {
        clearRedirectTimers();
        setError(null);
        setSuccess(false);

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

        if (result.hasError()) {
            setLoading(false);
            setError(result.error);
            return;
        }

        setLoading(false);
        setError(null);
        setSuccess(true);
        startRedirectCountdown();
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
                    disabled={loading || success}
                />

                <PasswordInput
                    placeholder="Password"
                    mt="md"
                    size="md"
                    radius="md"
                    value={password}
                    onChange={(e) => setPassword(e.currentTarget.value)}
                    disabled={loading || success}
                />

                <PasswordInput
                    placeholder="Confirm password"
                    mt="md"
                    size="md"
                    radius="md"
                    value={confirmPassword}
                    onChange={(e) => setConfirmPassword(e.currentTarget.value)}
                    disabled={loading || success}
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

                {success && (
                    <Alert
                        title="Signup successful"
                        color="green"
                        radius="sm"
                        mt="sm"
                        variant="light"
                    >
                        <Text>
                            Account created. Redirecting to login in
                        </Text>
                        <Center mt={6} mb={6}>
                            <Transition
                                mounted
                                transition="pop"
                                duration={180}
                                timingFunction="ease"
                                key={countdown}
                            >
                                {(styles) => (
                                    <Text
                                        style={styles}
                                        fw={500}
                                        fz={32}
                                        lh={1}
                                    >
                                        {countdown}
                                    </Text>
                                )}
                            </Transition>
                        </Center>
                        <br />

                        <Center>
                            <Anchor
                                href="#"
                                fw={500}
                                onClick={(event) => {
                                    event.preventDefault();
                                    navigateToLogin();
                                }}
                            >
                               Click here to login now
                            </Anchor>
                        </Center>
                    </Alert>
                )}

                <Button
                    fullWidth
                    mt="xl"
                    size="md"
                    radius="md"
                    className={classes.signupButton}
                    onClick={handleSignup}
                    disabled={loading || success}
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
