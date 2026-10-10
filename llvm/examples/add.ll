; First code-generation milestone. No target triple until LLVM recognizes ANC216.
define i16 @add(i16 %a, i16 %b) {
entry:
    %result = add i16 %a, %b
    ret i16 %result
}
