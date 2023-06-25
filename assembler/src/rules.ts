import { Token, TokenTypeName } from "./lexer";

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
    | "oneArgInstruction"
    | "twoArgsInstruction"
    | "expression"

export type RuleSpecifier = {
    name: RuleName,
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
    value: TokenTypeName
}

type RuleAttributeType =
    "identifier"
    | "literalNumber"
    | "literalString"
    | "keyword"

export class Rule {
    private name: RuleName;
    private attributes: RuleAttribute[];

    constructor(name: RuleName, attributes: RuleAttribute[]) {
        this.name = name;
        this.attributes = attributes;
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
}

export type RuleAttribute = {
    name: string;
    value: string;
    type: RuleAttributeType;
}

function getRuleSpecifier(rules: { name: RuleName, rules: string[] }[]) {
    let parsedRules: RuleSpecifier[] = [];
    for (let rule of rules) {

    }
    return parsedRules;
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
export const rules: RuleSpecifier[] = getRuleSpecifier([
    { name: "useAs", rules: ["use %i as %i \n", "use %i as %n \n"] },
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
    { name: "register", rules: ["%r"] },
    { name: "registerToRegister", rules: ["<register> , <register> \n"] },
    { name: "absolute", rules: ["& %i", "& %n"] },
    { name: "absoluteIndexed", rules: ["& %i %+- %r", "% %n %+- %r"] },
    { name: "indirect", rules: ["[ %i ]", "[ %n ]"] },
    { name: "indirect", rules: ["[ %i ] %+- %r", "[ %n ] %+- %r"] },
    { name: "relativeToPC", rules: ["* %n"]},
    { name: "instruction", rules: ["%x"] },
    { name: "twoArgsInstruction", rules: ["%x <registerToRegister>"] },
    { name: "oneArgInstruction", rules: ["%x <register|relativeToPC|relativeToSP>"] },
]);