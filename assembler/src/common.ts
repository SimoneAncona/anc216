type Interrupt = {
    type: string;
    message: string;
};

type ResultInterface<Type> = {
    value?: Type;
    error?: Interrupt;
};

class Result<Type> {
    private value: Type | undefined;
    private error: Interrupt | undefined;

    constructor(result: ResultInterface<Type>) {
        this.value = result.value
        this.error = result.error
    }

    getValue(): Type | undefined { return this.value; }

    catch(fn: (e: Interrupt | undefined) => void): Result<Type> { if (this.error !== undefined) fn(this.error); return this }

    finally<ReturnType>(fn: (value: Type | undefined) => ReturnType): ReturnType { return fn(this.value); }
}

