import colors from 'colors';
import { logErrorAndExit } from './src/console';
import fs from "fs";
import { assemble } from './src/assembler';
import path from 'path';

colors.enable();

const help = `
Usage:
    ${process.argv0} sourceFile ${"[outputFile]".yellow}

Flags:
    ${"--draw".cyan}    Set draw authorization in UALf header

    ${"--fs".cyan}      Set fs access authorization in UALf header

    ${"-h".cyan}
    ${"--help".cyan}    Shows this list

    ${"--head".cyan}    Insert the UALf file header

    ${"-l".cyan} ${"[string]".yellow}
    ${"--link".cyan} ${"[string]".yellow} Link an external library

    ${"--ref".cyan}     Insert the symbol table in UALf file header

    ${"--sys".cyan}     Set system privileges in UALf header

    ${"-v".cyan}
    ${"--version".cyan} Shows the version

    ${"-z".cyan}        Fill with zeros from 0x0000 to the first label
`;

const version = `
Assembly standard version   ${"1.0".green}
ANC216 ISA                  ${"1.0".green}
Assembler version           ${"1.0".green}
`;

if (process.argv.length < 3 || process.argv[2] === "--help" || process.argv[2] === "-h") {
	console.log(help);
	process.exit(0);
}

if (process.argv[2] === "--version" || process.argv[2] === "-v") {
    console.log(version);
    process.exit(0);
}

const flagsWithValues = [
    "-l", "--link"
]

type Flag = {
    name: string;
    value?: string;
}

let flags: Flag[] = [];
for (let i = 0; i < process.argv.length;) {
    if (!(process.argv[i].startsWith("-"))) {
        i++;
        continue;
    }
    if (flagsWithValues.includes(process.argv[i])) {
        
        if (process.argv[i + 1] === undefined) logErrorAndExit({type: "MissingParameter", message: `A value is required after ${process.argv[i]}.`});
        
        flags.push({name: process.argv[i], value: process.argv[i + 1]});
        process.argv.splice(i, 2);
        continue;
    }
    flags.push({name: process.argv[i]});
    process.argv.splice(i, 1);
}

let sourceFileName = process.argv[2] ?? logErrorAndExit({type: "MissingParameter", message: "The source file is missing."});
let outFileName = process.argv[3] ?? "a.out";

if (sourceFileName.startsWith("'") || sourceFileName.startsWith('"')) {
	sourceFileName = sourceFileName.substring(1, sourceFileName.length - 1);
}

if (outFileName.startsWith("'") || outFileName.startsWith('"')) {
	outFileName = outFileName.substring(1, outFileName.length - 1);
}

if (!fs.existsSync(sourceFileName)) {
    logErrorAndExit({type: "FileNotFound", message: `The file ${sourceFileName} does not exist.`});
}

assemble({name: path.basename(sourceFileName), path: path.resolve(sourceFileName), isMain: true}, fs.readFileSync(sourceFileName).toString(), {head: false, zeros: false, link: []});