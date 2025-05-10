import toast from "react-hot-toast";
//import { init } from '@tauri-apps/api/tauri';
import {invoke} from "@tauri-apps/api/core";

export class NovaApi {
    async dicom_open(path: string): Promise<void> {
        toast.success("response2");
        const response = await invoke<string>('greet', {name: path})
        console.log(response);
        toast.success(response);
    }
}