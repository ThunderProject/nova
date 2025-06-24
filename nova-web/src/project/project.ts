import {NovaApi} from "../nova_api/NovaApi.ts";

export class Project {
    static async open(file: string) {
        await NovaApi.openProject(file);
    }
}