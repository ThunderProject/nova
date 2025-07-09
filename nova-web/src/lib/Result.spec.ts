import { describe, it, expect } from "vitest";
import { ok, err, type Result } from "./Result";

describe("Result", () => {
    describe("ok()", () => {
        it("should create an Ok with the given value", () => {
            const result = ok(42);

            if (result.hasValue()) {
                expect(result.value).toBe(42);
            } else {
                throw new Error("Expected Ok but got Err");
            }

            expect(result.hasError()).toBe(false);
        });
    });

    describe("err()", () => {
        it("should create an Err with the given error", () => {
            const result = err("Something went wrong");

            if (result.hasError()) {
                expect(result.error).toBe("Something went wrong");
            } else {
                throw new Error("Expected Err but got Ok");
            }

            expect(result.hasValue()).toBe(false);
        });

        it("should support custom error types", () => {
            const errorObj = { code: 500, message: "Server error" };
            const result = err(errorObj);

            if (result.hasError()) {
                expect(result.error).toEqual(errorObj);
            } else {
                throw new Error("Expected Err but got Ok");
            }
        });
    });

    describe("type guards", () => {
        it("should distinguish Ok from Err", () => {
            const success: Result<number> = ok(123);
            const failure: Result<number> = err("error");

            if (success.hasValue()) {
                expect(success.value).toBe(123);
            } else {
                throw new Error("Expected Ok but got Err");
            }

            if (failure.hasError()) {
                expect(failure.error).toBe("error");
            } else {
                throw new Error("Expected Err but got Ok");
            }
        });
    });
});
