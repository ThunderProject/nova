import {CTPresetSchema} from "../schemas/CTWindowingPresetSchema.ts";
import {logger} from "../lib/Logger.ts";
import {FileSystem} from "../lib/FileSystem.ts";

export class CTWindowingPresetsLoader {
    public static async load(): Promise<Record<string, { width: number, level: number }> | null> {
        try {
            const path = "D:/repos/nova/nova-web/src/assets/CT/windowingPresets.json";
            const fileContents = await FileSystem.read(path);

            if(fileContents.hasError()) {
                logger.error(`WindowingPresetsLoader: Failed to read file ${fileContents.error}`);
                return null;
            }

            const parsed = JSON.parse(fileContents.value);
            const result = CTPresetSchema.safeParse(parsed);

            if(!result.success) {
                logger.error(`WindowingPresetsLoader: Failed to parse file contents`);
                return null;
            }

            //just take the first value
            return Object.entries(result.data).reduce((acc, [key, val]) => {
                acc[key] = {
                    level: val.level[0],
                    width: val.width[0]
                };

                return acc;
            }, {} as Record<string, { width: number, level: number }>);
        }
        catch (e) {
            logger.error(`WindowingPresetsLoader: Got exception trying to load preset file: ${e}`)
            return null;
        }
    }
}