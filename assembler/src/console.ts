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

export function logCodeErrorAndExit(error: Interrupt, code: Token[], column: number, line: number) {
    console.error(currentModule.path + ":" + line);
    console.error(error.type.red);
    console.error(`\t${error.message} at line ${line} and column ${column}\n`);
    if (code.length === 0) process.exit(1);
    process.stdout.write(`\t${String(code[0].line).padEnd(4, " ").cyan} | `);
    if (code[0].column != 1) process.stdout.write("... ");
    for (let token of code) {
        process.stdout.write(token.column === column && token.line === line ? token.value.red + " " : token.value + " ");
        if (token.value === "\n") process.stdout.write(`\t${String(token.line + 1).padEnd(4, " ").cyan} | `);
    }
    process.exit(1);
}