import React, {useRef, useEffect} from 'react';
import { useUIStore } from '../../stores/uiStore.ts';
import styles from './ResizableDoubleNavbar.module.css';

export function ResizableDoubleNavbar({ children }: { children: React.ReactNode }) {
    const { setSidebarWidth, sidebarWidth } = useUIStore();
    const isResizing = useRef(false);

    useEffect(() => {
        const handleMouseMove = (e: MouseEvent) => {
            if (isResizing.current) {
                const newWidth = Math.max(160, e.clientX);
                setSidebarWidth(newWidth);
            }
        };

        const stopResizing = () => {
            isResizing.current = false;
        };

        document.addEventListener('mousemove', handleMouseMove);
        document.addEventListener('mouseup', stopResizing);

        return () => {
            document.removeEventListener('mousemove', handleMouseMove);
            document.removeEventListener('mouseup', stopResizing);
        };
    }, [setSidebarWidth]);

    return (
        <div className={styles.container}>
            <div className={styles.sidebar} style={{ width: sidebarWidth }}>
                {children}
            </div>
            <div
                className={styles.resizer}
                onMouseDown={() => {
                    isResizing.current = true;
                }}
            />
        </div>
    );
}
