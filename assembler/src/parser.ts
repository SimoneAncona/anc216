import ohm from "ohm-js";
import fs from "fs";
import path from "path";

const ANC216grammar = ohm.grammar(fs.readFileSync(path.join(__dirname, "anc216grammar.ohm")).toString());

const matched = ANC216grammar.match(
    `section .text

    _code:
        load r0, 56
        load r1, 16
        call add_two_numbers
        load l0, 0
        load r0, 0
        syscall

    add_two_numbers:
        tran r3, r0
        ret

    `
);

console.log(matched.message)
