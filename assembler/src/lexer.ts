import { Result } from "./common";
import { logErrorAndExit } from "./console";

const literalNumbers: TokenType = { regex: /\b(\-?(0x[0-9a-fA-F]+|\d+|0o[0-7]+|0b[0-1]+))/gmi, type: "literalNumber" };
const literalStrings: TokenType = { regex: /".*"/gmi, type: "literalString" };
const identifiers: TokenType = { regex: /\b[a-zA-Z_@][a-zA-Z0-9_@]*\b/gmi, type: "identifier" };
const sectionNames: TokenType = { regex: /\.[a-zA-Z0-9_]+\b/gmi, type: "sectionName" };
const instructions: TokenType = { regex: /\b(add|and|call|careq|clrc|clri|clrn|clro|clrs|clrt|cmp|cpuid|dec|hreq|hwrite|inc|ireq|jeq|jge|jgr|jle|jls|jmp|jne|jnn|jno|jnz|jn|jo|jz|kill|lcpid|ldbp|ldsp|ldsr|load|neg|not|or|pareq|par|phbp|phpc|phsp|phsr|pobp|popc|pop|posp|posr|push|read|req|reset|ret|sbp|sehi|seli|seti|sets|sett|shl|shr|sign|sihi|sili|stbp|store|stp|stsp|stsr|sub|swap|syscall|tbp|tcpid|tehi|teli|tihi|tili|time|tran|trbp|trsp|trsr|trt|tstart|tstop|ttp|write|xor)\bg/gmi, type: "instruction" };
const reserved: TokenType = { regex: /\b(use|used|as|import|org|word|byte|if|else|elif|endif|sizeof|reserve|local|global|section|link)\b/gmi, type: "reserved" };
const specials: TokenType = { regex: /(&|\*|:|,|;|\$|\+|-|\*|\/|>|<|=)/gmi, type: "special" };
const registers: TokenType = { regex: /(r|l)[0-7]|sp/gmi, type: "register" };
const brackets: TokenType = { regex: /(\[|\]|\(|\))/gmi, type: "bracket" };
const newLines: TokenType = { regex: /\n/gmi, type: "newLine" };
const other: TokenType = { regex: /\S+/gmi, type: "other" };
const comments: RegExp = /;.*/gmi;

const regularExpressions = [
    literalNumbers,
    literalStrings,
    reserved,
    registers,
    sectionNames,
    identifiers,
    instructions,
    specials,
    brackets,
    newLines,
    other
];

type TokenType = {
    regex: RegExp;
    type: TokenTypeName;
}

export type TokenTypeName =
    "literalNumber"
    | "literalString"
    | "identifier"
    | "sectionName"
    | "instruction"
    | "reserved"
    | "special"
    | "register"
    | "bracket"
    | "newLine"
    | "other"

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
        tokens.push(...tokenizeLine(lines[i], i).catch((e) => { logErrorAndExit(e) }).getValue() as Token[]);
    }

    return new Result<Token[]>({ value: tokens });
}

function tokenizeLine(line: string, lineNumber: number): Result<Token[]> {
    if (line.length === 0) return new Result<Token[]>({ value: [] });
    let tokens: Token[] = [];

    for (let tokenType of regularExpressions) {
        let matched = Array.from(line.matchAll(tokenType.regex));
        if (tokenType.type === "other" && matched.length !== 0 && !tokenOverlap({
            line: lineNumber,
            column: matched[0].index ?? 0,
            type: tokenType.type,
            value: matched[0][0]
        }, tokens)) return new Result<Token[]>({
            error: {
                type: "SyntaxError",
                message: `Unexpected token '${matched[0][0]}' at line ${lineNumber} and column ${(matched[0].index ?? 0) + 1}`
            }
        });

        matched.forEach((match) => {
            let token = {
                line: lineNumber + 1,
                column: (match.index ?? 0) + 1,
                type: tokenType.type,
                value: match[0]
            };
            if (!tokenOverlap(token, tokens))
                tokens.push(token);
        });
    }

    tokens = tokens.sort((a, b) => a.column < b.column ? -1 : 1);
    tokens.push({
        line: lineNumber,
        column: tokens[tokens.length - 1] !== undefined ? tokens[tokens.length - 1].column + tokens[tokens.length - 1].value.length : 1,
        type: "newLine",
        value: "\n"
    });

    return new Result<Token[]>({ value: tokens });
}

function tokenOverlap(token: Token, tokens: Token[]) {
    for (let tk of tokens) {
        if (
            token.column >= tk.column && token.column < tk.column + tk.value.length
            || token.column <= tk.column && token.column + token.value.length > tk.column
        ) return true;
    }
    return false;
}