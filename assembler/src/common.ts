export type Interrupt = {
    type: string;
    message: string;
};

type ResultInterface<Type> = {
    value?: Type;
    error?: Interrupt;
};

export class Result<Type> {
    private value: Type | undefined;
    private error: Interrupt | undefined;

    constructor(result: ResultInterface<Type>) {
        this.value = result.value
        this.error = result.error
    }

    getValue(): Type | undefined { return this.value; }

    catch(fn: (e: Interrupt) => void): Result<Type> { if (this.error !== undefined) fn(this.error); return this; }

    finally<ReturnType>(fn: (value: Type | undefined) => ReturnType): ReturnType { return fn(this.value); }
}

export function fitsIn8(value: number) {
    return (
		value == 0 ||
		value < 0 && ((~value) & 0xFF) === ~value ||
		(value & 0xFF) === value
	);
}

export function fitsIn16(value: number) {
    return (
        value == 0 ||
		value < 0 && ((~value) & 0xFFFF) === ~value ||
		(value & 0xFFFF) === value
    );
}
