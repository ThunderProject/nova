import './App.css'
import Viewport from "./components/viewport/Viewport.tsx";
import { Toaster } from 'react-hot-toast'

function App() {
    const MainToolbar = () => (
        <div style={{
            width: '100%',
            height: '42px',
            backgroundColor: '#1c1f2b',
            borderBottom: '1px solid #2c2f39',
            color: '#bbb',
            fontSize: '14px',
            display: 'flex',
            alignItems: 'center',
            padding: '0 12px',
        }}>
            Nova Toolbar (placeholder)
        </div>
    );
    const ViewportWindow = () => (
        <div style={{ flex: 1, display: 'flex', backgroundColor: '#0d0f1a' }}>
            <div style={{ flex: 1 }}>
                <Viewport />
            </div>
        </div>
    );

    return (
        <div style={{display: 'flex', flexDirection: 'column', width: '100%', height: '100%'}}>
            <Toaster position="top-center"/>
            <MainToolbar/>
            <ViewportWindow/>
        </div>
    );
}

export default App;

