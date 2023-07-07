import fs from 'fs';
import * as ohm from "ohm-js";
import path from "path";
import { parse } from './parser';
import { preProcess } from './preProcessor';

export function assemble(filename: string) {
    const grammar = fs.readFileSync(path.join(__dirname, "anc216grammar.ohm")).toString();
	const ANC216grammar = ohm.grammar(grammar);
    const matches = parse(fs.readFileSync(filename).toString(), ANC216grammar);
    const semantics = ANC216grammar.createSemantics();
    preProcess(matches, semantics);
}