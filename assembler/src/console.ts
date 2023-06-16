import colors from 'colors';
import { Interrupt } from './common';
colors.enable();

export function logErrorAndExit(error: Interrupt) {
    console.error(error.type.red);
    console.error(`\t${error.message}\n`);
    process.exit(1);
}