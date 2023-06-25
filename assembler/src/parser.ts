import { Result } from "./common";
import { logCodeErrorAndExit, logErrorAndExit, pushCodeErrorStack } from "./console";
import { Token } from "./lexer";
import { Rule } from "./rules";
import { SyntaxRule, syntaxRules } from "./syntaxRules";

export function parse(tokens: Token[]): Rule[] {
    checkSyntaxRules(tokens);
    return generateRules(tokens);
}

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

function generateRules(tokens: Token[]) {
    return []
}
