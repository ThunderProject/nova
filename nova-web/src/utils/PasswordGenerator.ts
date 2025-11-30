export class PasswordGenerator {
    public static generate(length: number = 18): string {
        const charset = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()-_=+[]{}<>?';
        const randomValues = new Uint32Array(length);

        crypto.getRandomValues(randomValues);
        return Array.from(randomValues, (val) => charset[val % charset.length]).join('');
    }
}