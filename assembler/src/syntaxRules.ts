import { TokenTypeName } from "./lexer"

export type SyntaxRule = {
    type: "generic"
    after: TokenTypeName,
    canOnlyExistTokens: TokenTypeName[],
    canOnlyExistSpecific: string[],
} | {
    type: "specific"
    after: string,
    canOnlyExistTokens: TokenTypeName[],
    canOnlyExistSpecific: string[],
}

export const syntaxRules: SyntaxRule[] = [
    {
        type: "specific",
        after: "(",
        canOnlyExistTokens: ["literalNumber", "identifier"],
        canOnlyExistSpecific: ["+", "-", "$", "(", ")"]
    },
    {
        type: "specific",
        after: ")",
        canOnlyExistTokens: ["newLine"],
        canOnlyExistSpecific: [",", ")", "+", "-", "*", "/"]
    },
    {
        type: "generic",
        after: "literalNumber",
        canOnlyExistTokens: ["newLine"],
        canOnlyExistSpecific: [",", ")", "+", "-", "*", "/"]
    },
    {
        type: "generic",
        after: "literalString",
        canOnlyExistTokens: ["newLine"],
        canOnlyExistSpecific: [","]
    },
    {
        type: "specific",
        after: "$",
        canOnlyExistTokens: ["newLine"],
        canOnlyExistSpecific: ["+", "-", "+", "/"]
    },
    {
        type: "specific",
        after: "reserve",
        canOnlyExistTokens: ["literalNumber", "identifier"],
        canOnlyExistSpecific: []
    },
    {
        type: "specific",
        after: "word",
        canOnlyExistTokens: ["literalNumber", "identifier"],
        canOnlyExistSpecific: []
    },
    {
        type: "specific",
        after: "byte",
        canOnlyExistTokens: ["literalNumber", "identifier"],
        canOnlyExistSpecific: []
    },
    {
        type: "specific",
        after: "import",
        canOnlyExistTokens: ["literalString", "identifier"],
        canOnlyExistSpecific: []
    },
    {
        type: "specific",
        after: "link",
        canOnlyExistTokens: ["literalString", "identifier"],
        canOnlyExistSpecific: []
    },
    {
        type: "specific",
        after: "use",
        canOnlyExistTokens: ["identifier"],
        canOnlyExistSpecific: []
    },
    {
        type: "specific",
        after: "section",
        canOnlyExistTokens: ["sectionName"],
        canOnlyExistSpecific: []
    },
    {
        type: "specific",
        after: "if",
        canOnlyExistTokens: ["identifier", "literalNumber", "literalString"],
        canOnlyExistSpecific: ["used", "not"]
    },
    {
        type: "specific",
        after: "elif",
        canOnlyExistTokens: ["identifier", "literalNumber", "literalString"],
        canOnlyExistSpecific: ["used", "not"]
    },
    {
        type: "specific",
        after: "used",
        canOnlyExistTokens: ["identifier"],
        canOnlyExistSpecific: []
    },
    {
        type: "specific",
        after: "not",
        canOnlyExistTokens: ["identifier", "literalNumber", "literalString"],
        canOnlyExistSpecific: ["used"]
    },
    {
        type: "specific",
        after: "global",
        canOnlyExistTokens: ["identifier"],
        canOnlyExistSpecific: []
    },
    {
        type: "specific",
        after: "local",
        canOnlyExistTokens: ["identifier"],
        canOnlyExistSpecific: []
    },
    {
        type: "specific",
        after: "sizeof",
        canOnlyExistTokens: ["identifier"],
        canOnlyExistSpecific: []
    },
    {
        type: "specific",
        after: "+",
        canOnlyExistTokens: ["identifier", "literalNumber"],
        canOnlyExistSpecific: ["(", "$", "+", "-"]
    },
    {
        type: "specific",
        after: "-",
        canOnlyExistTokens: ["identifier", "literalNumber"],
        canOnlyExistSpecific: ["(", "$", "+", "-"]
    },
    {
        type: "specific",
        after: "*",
        canOnlyExistTokens: ["identifier", "literalNumber"],
        canOnlyExistSpecific: ["(", "$", "+", "-"]
    },
    {
        type: "specific",
        after: "/",
        canOnlyExistTokens: ["identifier", "literalNumber"],
        canOnlyExistSpecific: ["(", "$", "+", "-"]
    },
    {
        type: "specific",
        after: "as",
        canOnlyExistTokens: ["literalNumber", "identifier", "literalString"],
        canOnlyExistSpecific: ["(", "+", "-"]
    },
    {
        type: "specific",
        after: "endif",
        canOnlyExistTokens: ["newLine"],
        canOnlyExistSpecific: []
    },
    {
        type: "generic",
        after: "instruction",
        canOnlyExistTokens: ["literalNumber", "register", "identifier"],
        canOnlyExistSpecific: ["byte", "word", "(", "*", "[", "&", "+", "-"],
    },
    {
        type: "specific",
        after: "org",
        canOnlyExistTokens: ["literalNumber", "identifier"],
        canOnlyExistSpecific: []
    },
    {
        type: "generic",
        after: "sectionName",
        canOnlyExistTokens: ["newLine"],
        canOnlyExistSpecific: ["\n"]
    },
    {
        type: "specific",
        after: ",",
        canOnlyExistTokens: ["identifier", "reserved", "register", "literalNumber", "literalString", "bracket"],
        canOnlyExistSpecific: ["word", "byte", "+", "-", "*", "&"]
    }
]