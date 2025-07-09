import { describe, it, expect } from "vitest";
import { ObjectUtils } from "./Utils.ts";

describe("ObjectUtils", () => {
    describe("snakifyString", () => {
        it("should convert camelCase to snake_case", () => {
            expect(ObjectUtils.snakifyString("camelCaseTest")).toBe("camel_case_test");
        });

        it("should convert PascalCase to snake_case", () => {
            expect(ObjectUtils.snakifyString("PascalCaseTest")).toBe("pascal_case_test");
        });

        it("should handle single lowercase words", () => {
            expect(ObjectUtils.snakifyString("word")).toBe("word");
        });

        it("should handle acronyms correctly", () => {
            expect(ObjectUtils.snakifyString("HTTPServerError")).toBe("http_server_error");
            expect(ObjectUtils.snakifyString("getURLResponse")).toBe("get_url_response");
        });

        it("should handle empty string", () => {
            expect(ObjectUtils.snakifyString("")).toBe("");
        });
    });

    describe("snakifyObject", () => {
        it("should convert object keys to snake_case", () => {
            const input = { camelCaseKey: "value", PascalCaseKey: 42, simple: true };
            const expected = { camel_case_key: "value", pascal_case_key: 42, simple: true };

            expect(ObjectUtils.snakifyObject(input)).toEqual(expected);
        });

        it("should handle empty objects", () => {
            expect(ObjectUtils.snakifyObject({})).toEqual({});
        });

        it("should leave already snake_case keys unchanged", () => {
            const input = { already_snake_case: "value" };
            expect(ObjectUtils.snakifyObject(input)).toEqual(input);
        });

        it("should preserve values", () => {
            const input = { keyOne: 123, keyTwo: [1, 2, 3], keyThree: { nested: "yes" } };
            const result = ObjectUtils.snakifyObject(input);

            expect(result.key_one).toBe(123);
            expect(result.key_two).toEqual([1, 2, 3]);
            expect(result.key_three).toEqual({ nested: "yes" });
        });
    });
});