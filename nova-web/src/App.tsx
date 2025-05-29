import '@mantine/core/styles.css';

import { MantineProvider } from '@mantine/core';
import { theme } from './theme';
import {DoubleNavbar} from "./DoubleNavbar.tsx";
import {MainTabBar} from "./components/MainTabBar/MainTabBar.tsx";

export default function App() {
    return (
        <MantineProvider theme={theme}>
            <MainTabBar/>
        </MantineProvider>
    );
}