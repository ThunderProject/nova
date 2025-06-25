export class ObjectUtils {
    static snakifyString(input: string): string {
        return input
            .replace(/([a-z0-9])([A-Z])/g, "$1_$2") // camelCase → camel_Case
            .replace(/([A-Z]+)([A-Z][a-z])/g, "$1_$2") // PascalCase → Pascal_Case
            .toLowerCase(); // final → snake_case
    }

    static snakifyObject<T extends object> (obj: T): Record<string, T[keyof T]> {
        return Object.fromEntries(
            Object.entries(obj).map(([key, value]) => [ObjectUtils.snakifyString(key), value])
        );
    }
}