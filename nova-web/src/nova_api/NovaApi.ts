import toast from "react-hot-toast";
import {invoke} from "@tauri-apps/api/core";
import {logger} from "../lib/Logger.ts";

const NovaApiCommandNames = {
    ReadFileToString: 'read_file_to_string',
} as const;

type NovaCommands = {
    [NovaApiCommandNames.ReadFileToString]: { params: { name: string }; result: string };
};

async function invokeNovaCommand<K extends keyof NovaCommands>(
    command: K,
    params: NovaCommands[K]['params'],
): Promise<NovaCommands[K]['result']> {
    return invoke<NovaCommands[K]['result']>(command, params);
}

export class NovaApi {
    async dicom_open(path: string): Promise<void> {
        const response = await invoke<string>('greet', {name: path})
        toast.success(response);
    }

    public static async readFile(path: string): Promise<string | null> {
        try {
            return await invokeNovaCommand(NovaApiCommandNames.ReadFileToString, {name: path});
        }
        catch (error) {
            logger.error(`NovaApi: readFile got error: ${error}`);
            return null;
        }
    }
}