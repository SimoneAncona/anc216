import * as ohm from "ohm-js";
import fs from "fs";
import path from "path";

export function parse(source: string, grammar: ohm.Grammar) {
	source = source.replace(/;.*/gmi, "");

	let lines = source.split("\n");
	let matches = [];

	for (let line of lines) {
	    if (line.trim() === "") continue;
	    const matched = grammar.match(line);
	    if (matched.failed()) {
	        console.error(matched.message);
	        process.exit(1);
	    }
		matches.push(matched);
	}

	return matches;
}
