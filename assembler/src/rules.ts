import { Token, TokenTypeName } from "./lexer";
import { AssemblyObject } from "./parser";

type RuleName =
    "useAs"
    | "use"
    | "import"
    | "importAs"
    | "link"
    | "section"
    | "label"
    | "origin"
    | "localLabel"
    | "globalLabel"
    | "ifUsed"
    | "ifNotUsed"
    | "elifUsed"
    | "elifNotUsed"
    | "reserve"
    | "sizeof"
    | "wordCasting"
    | "byteCasting"
    | "register"
    | "absolute"
    | "absoluteIndexed"
    | "indirect"
    | "indirectIndexed"
    | "relativeToPC"
    | "relativeToSP"
    | "registerToRegister"
    | "instruction"
    | "immediate"
    | "registerToMemory"
    | "memoryToRegister"
    | "oneArgInstruction"
    | "twoArgsInstruction"
    | "labelAccess"
    | "expression"

export type RuleSpecifier = {
    name: RuleName,
    subRule: RuleSpecifier | null,
    args: RuleSpecifierArgument[][],
    restrict: boolean, // that means that if a rule does not follow the specific pattern it raises a SyntaxError
    // for example, if the rule use %i is restrict that means that is there is:
    // use 10
    // it raises a SyntaxError because 10 is not an identifier but a number
}

type RuleSpecifierArgument = {
    type: "specific",
    value: string
} | {
    type: "generic",
    value: TokenTypeName | "plusMinus"
}

type RuleAttributeType =
    "identifier"
    | "literalNumber"
    | "literalString"
    | "keyword"

export class Rule {
    private name: RuleName;
    private subRule: Rule | null;
    private attributes: RuleAttribute[];
    private initialToken: Token;
    private finalToken: Token;

    constructor(name: RuleName, subRule: Rule | null, attributes: RuleAttribute[], initialToken: Token, finalToken: Token) {
        this.name = name;
        this.subRule = subRule;
        this.attributes = attributes;
        this.initialToken = initialToken;
        this.finalToken = finalToken;
    }

    getName(): RuleName { return this.name; }

    getAttributes(): RuleAttribute[] {
        return this.attributes;
    }

    getAttribute(name: string): RuleAttribute | null {
        for (const attribute of this.attributes) {
            if (attribute.name === name) return attribute;
        };
        return null;
    }

    getInitialToken() { return this.initialToken; }

    getFinalToken() { return this.finalToken; }
}

export type RuleAttribute = {
    name: string;
    value: string;
    type: RuleAttributeType;
}

function getRuleSpecifier(rules: { name: RuleName, rules: string[] }[]) {
    let parsedRules: RuleSpecifier[] = [];
    for (let rule of rules) {
        let args: RuleSpecifierArgument[][] = [];
        for (let singleRule of rule.rules) {
            args.push(parseRuleSpecifier(singleRule.split(" ")));
        }
        parsedRules.push({ 
            name: rule.name,
            subRule: null,
            args: args,
            restrict: true,
        });
    }
    return parsedRules;
}

function parseRuleSpecifier(rule: string[]) {
    let ruleArgs: RuleSpecifierArgument[] = [];
    for (let str of rule) {
        if (str.startsWith("<")) {
            continue;
        }
        switch (str) {
            case "%i":
                ruleArgs.push({ type: "generic", value: "identifier" });
                break;
            case "%si":
                ruleArgs.push({ type: "generic", value: "sectionName" });
                break;
            case "%n":
                ruleArgs.push({ type: "generic", value: "literalNumber" });
                break;
            case "%s":
                ruleArgs.push({ type: "generic", value: "literalString" });
                break;
            case "%x":
                ruleArgs.push({ type: "generic", value: "instruction" });
                break;
            case "%+-":
                ruleArgs.push({ type: "generic", value: "plusMinus" });
                break;
            case "%r":
                ruleArgs.push({ type: "generic", value: "register" });
                break;
            default:
                ruleArgs.push({ type: "specific", value: str });
                break;
        }
    }
    return ruleArgs;
}

/**
 * The rule specifier format is the following:
 * use %i as %i \n
 * That specify the rule where are the:
 *  use keyword
 *  an identifier (%i)
 *  the as keyword
 *  another identifier
 *  the end of the line
 * 
 * %i   identifier
 * %si  section identifier
 * %n   number
 * %s   a string
 * %x   an instruction
 * %+-  + or -
 * %r   a register
 * <ruleName>   another rule
 * <rule|anotherRule> other rules
 * |    a separator, it can be , or a new line
 */
export const preProcessorRules: RuleSpecifier[] = getRuleSpecifier([
    { name: "useAs", rules: ["use %i as %i \n", "use %i as %n \n", "use %i as %s \n"] },
    { name: "use", rules: ["use %i \n"] },
    { name: "importAs", rules: ["import %i as %i \n"] },
    { name: "import", rules: ["import %i \n"] },
    { name: "link", rules: ["link %i \n"] },
    { name: "section", rules: ["section %si \n"] },
    { name: "label", rules: ["%i :"] },
    { name: "origin", rules: ["org %n \n"] },
    { name: "localLabel", rules: ["local <label>"] },
    { name: "globalLabel", rules: ["global <label>"] },
    { name: "ifUsed", rules: ["if used %i \n"] },
    { name: "ifNotUsed", rules: ["if not used %i \n", "if ! used %i \n"] },
    { name: "elifUsed", rules: ["elif used %i \n"] },
    { name: "elifNotUsed", rules: ["elif not used %i \n", "elif ! used %i \n"] },
    { name: "reserve", rules: ["reserve %i", "reserve %n"] },
    { name: "sizeof", rules: ["sizeof %i"] },
    { name: "wordCasting", rules: ["word %n", "word %i"] },
    { name: "byteCasting", rules: ["byte %n", "byte %i"] },
]);

export const rules: RuleSpecifier[] = getRuleSpecifier([
    { name: "register", rules: ["%r"] },
    { name: "registerToRegister", rules: ["<register> , <register> \n"] },
    { name: "absolute", rules: ["& %i", "& %n"] },
    { name: "absoluteIndexed", rules: ["& %i %+- %r", "% %n %+- %r"] },
    { name: "indirect", rules: ["[ %i ]", "[ %n ]"] },
    { name: "indirect", rules: ["[ %i ] %+- %r", "[ %n ] %+- %r"] },
    { name: "relativeToPC", rules: ["* %n", "* %+- %n", "* %i", "* %+- %i"] },
    { name: "relativeToSP", rules: ["& sp %+- %n", "& sp %+- %i"] },
    { name: "immediate", rules: ["%n", "%i"] },
    { name: "memoryToRegister", rules: ["<register> , <absolute|relativeToPC|relativeToSP|immediate> \n"] },
    { name: "registerToMemory", rules: ["<absolute|relativeToPC|relativeToSP|immediate> , <register> \n"] },
    { name: "instruction", rules: ["%x"] },
    { name: "labelAccess", rules: ["%i . %i"] },
    { name: "twoArgsInstruction", rules: ["%x <registerToRegister|registerToMemory|memoryToRegister>"] },
    { name: "oneArgInstruction", rules: ["%x <register|absolute|absoluteIndexed|indirect|indirectIndexed|relativeToPC|relativeToSP|immediate>"] },
]);

export function generateRules(objs: AssemblyObject[], ruleSpecifiers: RuleSpecifier[]) {
    let rules: Rule[] = [];
    return rules;
}