import { tokenize } from "../src/lexer"

const correctLexerTest1 = `
; Strings and constants

section .data
    message: “Hello, World!”
    message_size: $ - message

section .text
_code:
    load l0, 5
    load r1, message
    load r2, & message_size
    load r3, 0
    syscall

    load l0, 0
    load r0, 0
    syscall ; exit
`
const correctLexerTest2 = `
`;
const correctLexerTest3 = `
`;
const correctLexerTest4 = `
`;

const wrongLexerTest1 = `
impo#rt hello

`;

test("Lexer correct 1", () => {
    expect(correctLexerTest1)
});

test("Lexer incorrect 1", () => {
    
})