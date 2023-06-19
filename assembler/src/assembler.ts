import { logErrorAndExit } from "./console";
import { Token, tokenize } from "./lexer";
import { ModuleInfo, registerModule, setCurrentModule } from "./linker";
import { parse } from "./parser";
import { Rule } from "./rules";

type AssembleOptions = {
    head: false;
    zeros: boolean;
    link: string[];
} | {
    head: true;
    draw: boolean;
    sys: boolean;
    fs: boolean;
    zeros: boolean;
    link: string[];
};

export function assemble(module: ModuleInfo, source: string, options: AssembleOptions) {
    setCurrentModule(module);
    registerModule(module);

    let tokens = tokenize(source).catch((e) => {
        logErrorAndExit(e);
    }).getValue() as Token[];

    let rules = parse(tokens).catch((e) => {
        logErrorAndExit(e);
    }).getValue() as Rule[];

}