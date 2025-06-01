import { z } from 'zod';

export const CTPresetSchema = z.record(
    z.string(),
    z.object({
        level: z.array(z.number()),
        width: z.array(z.number()),
    })
);

export type RawCTPresetData = z.infer<typeof CTPresetSchema>