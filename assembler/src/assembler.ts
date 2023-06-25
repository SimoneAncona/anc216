import { displayErrorStackIfNecessary, logErrorAndExit, pushCodeErrorStack } from "./console";
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

    let tokens = tokenize(source);
    displayErrorStackIfNecessary();
    let rules = parse(tokens);
    displayErrorStackIfNecessary();

}