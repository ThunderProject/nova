import { createBrowserRouter, RouterProvider } from 'react-router-dom';
import { HomePage } from './pages/home.tsx';

const router = createBrowserRouter([
    {
        element: <HomePage />,
        path: '/',
    },
]);

export function Router() {
    return <RouterProvider router={router} />;
}