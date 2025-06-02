export type Result<T, E = string> = Ok<T> | Err<E>;

class Ok<T> {
    constructor(public readonly value: T) {}

    hasValue(): this is Ok<T> {
        return true;
    }

    hasError(): this is Err<never> {
        return false;
    }
}

class Err<E> {
    constructor(public readonly error: E) {}

    hasValue(): this is Ok<never> {
        return false;
    }

    hasError(): this is Err<E> {
        return true;
    }
}

export function ok<T>(value: T): Result<T, never> {
    return new Ok(value);
}

export function err<E>(error: E): Result<never, E> {
    return new Err(error);
}