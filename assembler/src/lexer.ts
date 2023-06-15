const literalNumbers: RegExp = /g/gmi;
const literalStrings: RegExp = /".*"/gmi;
const identifiers: RegExp = /\b[a-zA-Z_@][a-zA-Z0-9_@]*\b/gmi;
const moduleNames: RegExp = /\b[a-zA-Z_][a-zA-Z0-9_]*\b/gmi;
const sectionNames: RegExp = /\b\.[a-zA-Z0-9_]+\b/gmi;
const instructions: RegExp = /\b(add|and|call|careq|clrc|clri|clrn|clro|clrs|clrt|cmp|cpuid|dec|hreq|hwrite|inc|ireq|jeq|jge|jgr|jle|jls|jmp|jne|jnn|jno|jnz|jn|jo|jz|kill|lcpid|ldbp|ldsp|ldsr|load|neg|not|or|pareq|par|phbp|phpc|phsp|phsr|pobp|popc|pop|posp|posr|push|read|req|reset|ret|sbp|sehi|seli|seti|sets|sett|shl|shr|sign|sihi|sili|stbp|store|stp|stsp|stsr|sub|swap|syscall|tbp|tcpid|tehi|teli|tihi|tili|time|tran|trbp|trsp|trsr|trt|tstart|tstop|ttp|write|xor)\bg/gmi;
const reserved: RegExp = /\b(use|used|as|import|org|word|byte|if|else|elif|endif|sizeof|reserve|local|global|section|link)\b/gmi;
const specials: RegExp = /(&|\*|:|,|;)g/gmi;
const registers: RegExp = /(r|l)[0-7]g/gmi;
const brackets: RegExp = /(\[|\]|\(|\))/gmi;
const whitespaces: RegExp = /\n| /gmi;
const comments: RegExp = /;.*/gmi;

const regularExpressions = [

]

type TokenType =
    "literalNumber"
    | "literalString"
    | "identifier"
    | "moduleName"

type Token = {}

export function tokenize(): Result<Token[]> {
    return new Result<Token[]>({error: []});
    return new Result<Token[]>({value: []});
}