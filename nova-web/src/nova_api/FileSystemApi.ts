import {logger} from "../lib/Logger.ts";
import {invokeNovaCommand, NovaCommand,} from "./NovaApi.ts";
import {err, ok, type Result} from "../lib/Result.ts";

export class NovaFileSystemApi {
    public static async read(path: string): Promise<Result<string>> {
        try {
            return ok(await invokeNovaCommand(NovaCommand.ReadFileToString, {file: path}));
        }
        catch (error) {
            const errMsg: string = `Failed to read file "${path}". Reason: ${error}`;
            logger.error(errMsg);
            return err(errMsg);
        }
    }

    public static async write(path: string, contents: string): Promise<boolean> {
        try {
            return await invokeNovaCommand(NovaCommand.WriteFile, {path: path, contents: contents});
        }
        catch (error) {
            const errMsg: string = `Failed to write contents to file "${path}". Reason: ${error}`;
            logger.error(errMsg);
            return false;
        }
    }

    public static async createDir(dir: string): Promise<boolean> {
        try {
            return await invokeNovaCommand(NovaCommand.CreateDir, {dir: dir});
        }
        catch (error) {
            const errMsg: string = `Failed to create directory 1${dir}1. Reason: ${error}`;
            logger.error(errMsg);
            return false;
        }
    }

    public static async createDirRecursive(dir: string): Promise<boolean> {
        try {
            return await invokeNovaCommand(NovaCommand.CreateDirRecursive, {dir: dir});
        }
        catch (error) {
            const errMsg: string = `Failed to create directory "${dir}". Reason: ${error}`;
            logger.error(errMsg);
            return false;
        }
    }

    public static async removeDir(dir: string): Promise<boolean> {
        try {
            return await invokeNovaCommand(NovaCommand.RemoveDir, {dir: dir});
        }
        catch (error) {
            const errMsg: string = `Failed to remove directory "${dir}". Reason: ${error}`;
            logger.error(errMsg);
            return false;
        }
    }

    public static async removeDirRecursive(dir: string): Promise<boolean> {
        try {
            return await invokeNovaCommand(NovaCommand.RemoveDirRecursive, {dir: dir});
        }
        catch (error) {
            const errMsg: string = `Failed to remove directory "${dir}". Reason: ${error}`;
            logger.error(errMsg);
            return false;
        }
    }

    public static async removeFile(file: string): Promise<boolean> {
        try {
            return await invokeNovaCommand(NovaCommand.RemoveFile, {file: file});
        }
        catch (error) {
            const errMsg: string = `Failed to remove file "${file}". Reason: ${error}`;
            logger.error(errMsg);
            return false;
        }
    }

    public static async rename(from: string, to: string): Promise<boolean> {
        try {
            return await invokeNovaCommand(NovaCommand.RenamePath, {from: from, to: to});
        }
        catch (error) {
            const errMsg: string = `Failed to rename "${from}" to "${to}". Reason: ${error}`;
            logger.error(errMsg);
            return false;
        }
    }

    public static async exist(path: string): Promise<boolean> {
        try {
            return await invokeNovaCommand(NovaCommand.PathExists, {path: path});
        }
        catch (error) {
            const errMsg: string = `Failed to check if path "${path}" exists". Reason: ${error}`;
            logger.error(errMsg);
            return false;
        }
    }

    public static async isEmpty(path: string): Promise<boolean> {
        try {
            return await invokeNovaCommand(NovaCommand.IsEmpty, {path: path});
        }
        catch (error) {
            const errMsg: string = `Failed to check if folder "${path}" is empty". Reason: ${error}`;
            logger.error(errMsg);
            return false;
        }
    }
}