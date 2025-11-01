import { StrictMode } from 'react';
import ReactDOM from 'react-dom/client';
import {BrowserRouter} from "react-router";
import App from './App';

ReactDOM.createRoot(document.querySelector('#root')!).render(
    <StrictMode>
        <BrowserRouter>
            <App/>
        </BrowserRouter>
    </StrictMode>
);
