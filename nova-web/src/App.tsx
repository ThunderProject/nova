import '@mantine/core/styles.css';

import { MantineProvider } from '@mantine/core';
import { theme } from './theme';
import {MainTabBar} from "./components/MainTabBar/MainTabBar.tsx";
import {Route, Routes} from "react-router";
import Viewer from './pages/viewer';
import { Toaster } from 'react-hot-toast';
import {ModalsProvider} from "@mantine/modals";
import {PageNotFound} from "./pages/not_found/PageNotFound.tsx";

export default function App() {
    return (
        <MantineProvider
            theme={theme}
            defaultColorScheme="dark"
        >
            <ModalsProvider>
                <Toaster/>
                <MainTabBar/>
                <Routes>
                    <Route path="/viewer" element={<Viewer />} />
                    {/*<Route path="/viewer" element={<Viewer />} />*/}
                    {/*<Route path="/patients" element={<Patients />} />*/}
                    {/*<Route path="/export" element={<Export />} />*/}
                    <Route path="*" element={<PageNotFound />} />
                </Routes>
            </ModalsProvider>
        </MantineProvider>
    );
}