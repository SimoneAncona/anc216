import colors from 'colors';
import { Interrupt } from './common';
import { Token } from './lexer';
import { currentModule } from './linker';
colors.enable();

export function logErrorAndExit(error: Interrupt) {
    console.error(error.type.red);
    console.error(`\t${error.message}\n`);
    process.exit(1);
}

type CodeError = {
    error: Interrupt,
    tokens: Token[],
    column: number,
    line: number
}

let errorStack: CodeError[] = [];

function logCodeError(error: Interrupt, code: Token[], column: number, line: number) {
    console.error(currentModule.path + ":" + line + ":" + column);
    console.error(error.type.red);
    console.error(`\t${error.message} at line ${line} and column ${column}\n`);
    if (code.length === 0) return;
    process.stdout.write(`\t${String(code[0].line).padEnd(4, " ").cyan} | `);
    if (code[0].column != 1) process.stdout.write("...");
    for (let token of code) {
        if (!(token.value === "," || token.value === ".")) process.stdout.write(" ");
        process.stdout.write(token.column === column && token.line === line ? token.value.red : token.value);
        if (token.value === "\n") process.stdout.write(`\t${String(token.line + 1).padEnd(4, " ").cyan} | `);
    }
}

export function logCodeErrorAndExit(error: Interrupt, code: Token[], column: number, line: number) {
    logCodeError(error, code, column, line);
    process.exit(1);
}

export function pushCodeErrorStack(error: Interrupt, code: Token[], column: number, line: number) {
    errorStack.push({error: error, tokens: code, column: column, line: line});
}

export function displayErrorStackIfNecessary() {
    if (errorStack.length === 0) return;
    for (let error of errorStack) {
        logCodeError(error.error, error.tokens, error.column, error.line);
        console.log("\n");
    }
    process.exit(1);
}