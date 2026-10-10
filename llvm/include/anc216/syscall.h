#ifndef ANC216_SYSCALL_H
#define ANC216_SYSCALL_H

/* args[0..4]: R1..R5 inputs; args[5]: R6 output.
 * Returns R7 (zero on success, OS error otherwise).
 * Always provide six words, even for a service with no arguments.
 * Pointer arguments are logical user IMEM offsets.
 */
extern int syscall(unsigned service, unsigned *args);

#endif
