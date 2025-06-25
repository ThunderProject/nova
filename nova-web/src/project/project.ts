import {NovaApi, type ProjectParams} from "../nova_api/NovaApi.ts";

export class Project {
    static async createNewProject(params: ProjectParams) {
        await NovaApi.createNewProject(params);
    }

    static async open(file: string) {
        await NovaApi.openProject(file);
    }
}