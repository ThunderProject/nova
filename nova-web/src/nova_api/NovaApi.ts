import toast from "react-hot-toast";
import {invoke} from "@tauri-apps/api/core";
import {logger} from "../lib/Logger.ts";

export const NovaCommand = {
    ReadFileToString: 'read_file_to_string',
    CreateDir: 'create_dir',
    CreateDirRecursive: 'create_dir_recursive',
    RemoveDir: 'remove_dir',
    RemoveDirRecursive: 'remove_dir_recursive',
    RemoveFile: 'remove_file',
    RenamePath: 'rename_path',
    PathExists: 'path_exists',
    WriteFile: 'write_file',
    OpenProject: 'open_project',
    IsEmpty: 'is_empty',
    Log: 'log'
} as const;

type NovaCommandMap = {
    [NovaCommand.ReadFileToString]: { params: { file: string }; result: string };
    [NovaCommand.CreateDir]: { params: { dir: string }; result: boolean };
    [NovaCommand.CreateDirRecursive]: { params: { dir: string }; result: boolean };
    [NovaCommand.RemoveDir]: { params: { dir: string }; result: boolean };
    [NovaCommand.RemoveDirRecursive]: { params: { dir: string }; result: boolean };
    [NovaCommand.RemoveFile]: { params: { file: string }; result: boolean };
    [NovaCommand.RenamePath]: { params: { from: string; to: string }; result: boolean };
    [NovaCommand.PathExists]: { params: { path: string }; result: boolean };
    [NovaCommand.WriteFile]: { params: { path: string; contents: string }; result: boolean };
    [NovaCommand.OpenProject]: { params: { file: string; }; result: void };
    [NovaCommand.IsEmpty]: { params: { path: string; }; result: boolean };
    [NovaCommand.Log]: { params: { level: string; msg: string }; result: void };
};

export async function invokeNovaCommand<K extends keyof NovaCommandMap>(
    command: K,
    params: NovaCommandMap[K]['params'],
): Promise<NovaCommandMap[K]['result']> {
    return invoke<NovaCommandMap[K]['result']>(command, params);
}

export class NovaApi {
    async dicom_open(path: string): Promise<void> {
        const response = await invoke<string>('greet', {name: path})
        toast.success(response);
    }

    static async openProject(file: string): Promise<void> {
        try {
            return await invokeNovaCommand(NovaCommand.OpenProject, {file: file});
        }
        catch (error) {
            const errMsg: string = `Failed to open project "${file}". Reason: ${error}`;
            logger.error(errMsg);
        }
    }

    static Log(level: string, msg: string): void {
        try {
            return void invokeNovaCommand(NovaCommand.Log, {level: level, msg: msg});
        }
        catch (error) {
            void error;
        }
    }
}