import {NovaFileSystemApi} from "../nova_api/FileSystemApi.ts";
import {type Result} from "./Result.ts";

export class FileSystem {
    public static async read(path: string): Promise<Result<string>> {
        return NovaFileSystemApi.read(path);
    }

    public static async write(path: string, contents: string): Promise<boolean> {
        return NovaFileSystemApi.write(path, contents);
    }

    public static async createDir(path: string): Promise<boolean> {
        return NovaFileSystemApi.createDir(path);
    }

    public static async createDirRecursive(path: string): Promise<boolean> {
        return NovaFileSystemApi.createDirRecursive(path);
    }

    public static async removeDir(dir: string): Promise<boolean> {
        return NovaFileSystemApi.removeDir(dir);
    }

    public static async removeDirRecursive(dir: string): Promise<boolean> {
        return NovaFileSystemApi.removeDirRecursive(dir);
    }

    public static async removeFile(file: string): Promise<boolean> {
        return NovaFileSystemApi.removeFile(file);
    }

    public static async rename(from: string, to: string): Promise<boolean> {
        return NovaFileSystemApi.rename(from, to);
    }

    public static async exist(path: string): Promise<boolean> {
        return NovaFileSystemApi.exist(path);
    }

    public static async isEmpty(path: string): Promise<boolean> {
        return NovaFileSystemApi.isEmpty(path);
    }

    public static async join(parts: string[]): Promise<string> {
        return NovaFileSystemApi.join(parts);
    }
}