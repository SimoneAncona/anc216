import { Result } from "./common";
import { Token } from "./lexer";
import { Rule } from "./rules";

export function parse(tokens: Token[]): Result<Rule[]> {
    return new Result<Rule[]>({value: [new Rule("", [])]});
}