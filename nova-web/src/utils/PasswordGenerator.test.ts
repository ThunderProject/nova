import { describe, it, expect } from "vitest";
import { PasswordGenerator } from "./PasswordGenerator";

describe("PasswordGenerator", () => {
    it("should generate a password of the specified length", () => {
        const length = 24;
        const password = PasswordGenerator.generate(length);
        expect(password).toHaveLength(length);
    });

    it("should generate a password with default length 18", () => {
        const password = PasswordGenerator.generate();
        expect(password).toHaveLength(18);
    });

    it("should only contain allowed characters", () => {
        const allowed = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()-_=+[]{}<>?";
        const password = PasswordGenerator.generate(1000);

        for (const ch of password) {
            expect(allowed.includes(ch)).toBe(true);
        }
    });

    it("should generate different passwords each time", () => {
        const a = PasswordGenerator.generate();
        const b = PasswordGenerator.generate();

        expect(a).not.toBe(b);
    });

    it("should produce uniformly distributed characters", () => {
        const data = Array.from({ length: 10000 }, () => PasswordGenerator.generate(1)).join("");
        expect(/[A-Z]/.test(data)).toBe(true);
        expect(/[a-z]/.test(data)).toBe(true);
        expect(/[0-9]/.test(data)).toBe(true);
        expect(/[!@#$%^&*()[\]{}\-_=+<>?]/.test(data)).toBe(true);
    });
});
