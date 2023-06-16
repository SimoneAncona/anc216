import colors from 'colors';
colors.enable();

export function logErrorAndExit(error: Interrupt) {
    console.error(error.type.red);
    console.error(`\t${error.message}`);
    process.exit(1);
}