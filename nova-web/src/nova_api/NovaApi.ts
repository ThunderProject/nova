import toast from "react-hot-toast";
import {invoke} from "@tauri-apps/api/core";
import {logger} from "../lib/Logger.ts";
import {ObjectUtils} from "../lib/Utils.ts";
import {err, ok, type Result} from "../lib/Result.ts";

export const NovaCommand = {
    CreateDir: 'create_dir',
    CreateDirRecursive: 'create_dir_recursive',
    CreateNewProject: 'create_new_project',
    IsEmpty: 'is_empty',
    Join: "join",
    Log: 'log',
    OpenProject: 'open_project',
    PathExists: 'path_exists',
    ReadFileToString: 'read_file_to_string',
    RemoveDir: 'remove_dir',
    RemoveDirRecursive: 'remove_dir_recursive',
    RemoveFile: 'remove_file',
    RenamePath: 'rename_path',
    WriteFile: 'write_file',
    Login: 'login',
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
    [NovaCommand.CreateNewProject]: { params: { params: MappedProjectParams; }; result: void };
    [NovaCommand.IsEmpty]: { params: { path: string; }; result: boolean };
    [NovaCommand.Log]: { params: { level: string; msg: string }; result: void };
    [NovaCommand.Join]: { params: { parts: string[] }; result: string };
    [NovaCommand.Login]: { params: { username: string; password: string }; result: void };
};

export async function invokeNovaCommand<K extends keyof NovaCommandMap>(
    command: K,
    params: NovaCommandMap[K]['params'],
): Promise<NovaCommandMap[K]['result']> {
    return invoke<NovaCommandMap[K]['result']>(command, params);
}

export interface ProjectParams {
    projectName: string;
    workingDirectory: string;
    importedFiles: string[];
}

//rust backend uses snake_case for param names
type MappedProjectParams = {
    project_name: string;
    working_directory: string;
    imported_files: string[];
};

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

    static async createNewProject(params: ProjectParams): Promise<void> {
        try {
            const rustParams: MappedProjectParams = ObjectUtils.snakifyObject(params) as MappedProjectParams;
            return await invokeNovaCommand(NovaCommand.CreateNewProject, {params: rustParams});
        }
        catch (error) {
            const errMsg: string = `Failed to create new project. Reason: ${error}`;
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

    static async login(username: string, password: string): Promise<Result<void>> {
        try {
            await invokeNovaCommand(NovaCommand.Login, { username, password });
            return ok<void>(undefined);
        } catch (error) {
            //guaranteed from backend
            const msg = error as string

            logger.error(`Login failed: ${msg}`);
            return err<string>(msg);
        }
    }
}