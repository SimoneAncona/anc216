import { Result, fitsIn8 } from "./common";
import { logCodeErrorAndExit, logErrorAndExit, pushCodeErrorStack } from "./console";
import { Token } from "./lexer";
import { Rule } from "./rules";
import { SyntaxRule, syntaxRules } from "./syntaxRules";

export function parse(tokens: Token[]) {
    checkSyntaxRules(tokens);
    return generateObjects(tokens);
}

export type AssemblyObject = {
    token: Token,
    type: "number",
    value: number,
    size: 8 | 16
} | {
    token: Token,
    type: "string",
    value: string,
    size: number
} | {
    token: Token,
    type: "instruction",
    value: string,
    size: number
} | {
    token: Token,
    type: "other",
    value: number,
    size: number
};

function checkSyntaxRules(tokens: Token[]) {
    for (let i = 0; i < tokens.length - 1; i++) {
        for (let syntax of syntaxRules) {
            if (syntax.type === "specific") handleSyntaxRules("specific", tokens[i], tokens[i + 1], syntax);
            else handleSyntaxRules("generic", tokens[i], tokens[i + 1], syntax);
        }
    }
}

function handleSyntaxRules(type: "specific" | "generic", token1: Token, token2: Token, syntaxRule: SyntaxRule) {
    if (token1.value !== syntaxRule.after && type === "specific") return;
    if (token1.type !== syntaxRule.after && type === "generic") return;
    if (!syntaxRule.canOnlyExistSpecific.includes(token2.value) && !syntaxRule.canOnlyExistTokens.includes(token2.type)) pushCodeErrorStack({
        type: "SyntaxError",
        message: token2.value === "\n" ? "Unexpected end of line" : "Unexpected token"
    }, [token1, token2], token2.column, token2.line);
}

function generateObjects(tokens: Token[]) {
    let objects: AssemblyObject[] = [];
    for (let token of tokens) {
        switch (token.type) {
            case "literalNumber":
                objects.push({
                    token: token,
                    type: "number",
                    value: Number(token.value),
                    size: fitsIn8(Number(token.value)) ? 8 : 16
                });
                break;
            case "literalString":
                objects.push({
                    token: token,
                    type: "string",
                    value: token.value.substring(1, token.value.length - 1),
                    size: token.value.length - 2
                });
                break;
            case "instruction":
                objects.push({
                    token: token,
                    type: "instruction",
                    value: token.value,
                    size: 2
                });
                break;
            default:
                objects.push({
                    token: token,
                    type: "other",
                    value: 0,
                    size: 0
                });
                break;
        }
    }

    return objects;
}
