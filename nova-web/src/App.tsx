import '@mantine/core/styles.css';

import { MantineProvider } from '@mantine/core';
import { theme } from './theme';
import {MainTabBar} from "./components/MainTabBar/MainTabBar.tsx";
import {Route, Routes} from "react-router";
import Viewer from './pages/viewer';

export default function App() {
    return (
        <MantineProvider theme={theme}>
            <MainTabBar/>
            <Routes>
                <Route path="/viewer" element={<Viewer />} />
                {/*<Route path="/viewer" element={<Viewer />} />*/}
                {/*<Route path="/patients" element={<Patients />} />*/}
                {/*<Route path="/export" element={<Export />} />*/}
            </Routes>
        </MantineProvider>
    );
}