import '@mantine/core/styles.css';

import { MantineProvider } from '@mantine/core';
import {ModalsProvider} from "@mantine/modals";
import {Navigate, Route, Routes, useLocation} from "react-router";
import { Toaster } from 'react-hot-toast';
import React, {useEffect, useState} from "react";
import { theme } from './theme';
import Viewer from './pages/viewer';
import {PageNotFound} from "./pages/not_found/PageNotFound.tsx";
import {AuthenticationPage} from "./pages/authentication/AuthenticationPage.tsx";
import {NovaApi} from "./nova_api/NovaApi.ts";
import {MainTabBar} from "./components/MainTabBar/MainTabBar.tsx";

function AuthGuard({ isAuth, children }: { isAuth: boolean; children: React.ReactNode }) {
    if(!isAuth) {
        return <Navigate to="/login" replace />;
    }
    return <>{children}</>;
}

export default function App() {
    const [checked, setChecked] = useState(false);
    const [isAuth, setIsAuth] = useState(false);
    const loc = useLocation();

    useEffect(() => {
        (async () => {
            const result = await NovaApi.isAuthenticated();
            setIsAuth(result);
            setChecked(true);
        })();
    }, []);


    if(!checked) {
        return null
    }

    return (
        <MantineProvider
            theme={theme}
            defaultColorScheme="dark"
        >
            <ModalsProvider>
                <Toaster/>

                {loc.pathname !== "/login" && <MainTabBar />}

                <Routes>
                    <Route
                        path="/login"
                        element={ isAuth ? <Navigate to="/viewer" replace /> : <AuthenticationPage /> }
                    />

                    <Route
                        path="/viewer"
                        element={
                            <AuthGuard isAuth={isAuth}>
                                <Viewer />
                            </AuthGuard>
                        }
                    />

                    <Route
                        path="/"
                        element={
                            <AuthGuard isAuth={isAuth}>
                                <Viewer />
                            </AuthGuard>
                        }
                    />

                    <Route path="*" element={<PageNotFound />} />
                </Routes>

            </ModalsProvider>
        </MantineProvider>
    );
}