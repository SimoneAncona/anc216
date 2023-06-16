import colors from 'colors';
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