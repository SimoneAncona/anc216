import { Result } from "./common";
import { logErrorAndExit } from "./console";

const literalNumbers: TokenType = { regex: /\b(\-?(0x[0-9a-fA-F]+|\d+|0o[0-7]+|0b[0-1]+))/gmi, type: "literalNumber" };
const literalStrings: TokenType = { regex: /".*"/gmi, type: "literalString" };
const identifiers: TokenType = { regex: /\b[a-zA-Z_@][a-zA-Z0-9_@]*\b/gmi, type: "identifier" };
const moduleNames: TokenType = { regex: /\b[a-zA-Z_][a-zA-Z0-9_]*\b/gmi, type: "moduleName" };
const sectionNames: TokenType = { regex: /\b\.[a-zA-Z0-9_]+\b/gmi, type: "sectionName" };
const instructions: TokenType = { regex: /\b(add|and|call|careq|clrc|clri|clrn|clro|clrs|clrt|cmp|cpuid|dec|hreq|hwrite|inc|ireq|jeq|jge|jgr|jle|jls|jmp|jne|jnn|jno|jnz|jn|jo|jz|kill|lcpid|ldbp|ldsp|ldsr|load|neg|not|or|pareq|par|phbp|phpc|phsp|phsr|pobp|popc|pop|posp|posr|push|read|req|reset|ret|sbp|sehi|seli|seti|sets|sett|shl|shr|sign|sihi|sili|stbp|store|stp|stsp|stsr|sub|swap|syscall|tbp|tcpid|tehi|teli|tihi|tili|time|tran|trbp|trsp|trsr|trt|tstart|tstop|ttp|write|xor)\bg/gmi, type: "instruction" };
const reserved: TokenType = { regex: /\b(use|used|as|import|org|word|byte|if|else|elif|endif|sizeof|reserve|local|global|section|link)\b/gmi, type: "reserved" };
const specials: TokenType = { regex: /(&|\*|:|,|;)g/gmi, type: "special" };
const registers: TokenType = { regex: /(r|l)[0-7]g/gmi, type: "register" };
const brackets: TokenType = { regex: /(\[|\]|\(|\))/gmi, type: "bracket" };
const newLines: TokenType = { regex: /\n/gmi, type: "newLine" };
const comments: RegExp = /;.*/gmi;

const regularExpressions = [
    literalNumbers,
    literalStrings,
    reserved,
    registers,
    sectionNames,
    identifiers,
    moduleNames,
    instructions,
    specials,
    brackets,
    newLines,
];

type TokenType = {
    regex: RegExp;
    type: TokenTypeName;
}

export type TokenTypeName =
    "literalNumber"
    | "literalString"
    | "identifier"
    | "moduleName"
    | "sectionName"
    | "instruction"
    | "reserved"
    | "special"
    | "register"
    | "bracket"
    | "newLine"

export type Token = {
    line: number;
    column: number;
    type: TokenTypeName;
    value: string;
}

export function tokenize(source: string): Result<Token[]> {
    let tokens: Token[] = [];
    let lines: string[];

    source = source.replace(comments, "");
    lines = source.split("\n");

    for (let i = 0; i < lines.length; i++) {
        tokens.push(...tokenizeLine(lines[i], i).catch((e) => { logErrorAndExit(e)} ).getValue() as Token[]);
    }

    return new Result<Token[]>({ value: tokens });
}

function tokenizeLine(line: string, lineNumber: number): Result<Token[]> {
    if (line.length === 0) return new Result<Token[]>({value: []});
    let tokens: Token[] = [];

    for (let tokenType of regularExpressions) {
        let matched = Array.from(line.matchAll(tokenType.regex));
        if (matched.length === 0) continue;

        matched.forEach((token) => {

        });
    }

    return new Result<Token[]>({value: tokens});
}