;; Freestanding entry point (no CRT).
;; Calls: int intro_main(int argv, char *argvc)
;; Then exits with the returned value via the exit syscall.

extern intro_main

global _start

BITS 64

section .text

_start:
    ; rsp is 16-byte aligned on entry, as required by the SysV ABI.
    xor rdi, rdi          ; int argv -> 0
    xor rsi, rsi          ; char *argvc -> NULL
    call intro_main wrt ..plt

    mov edi, eax          ; exit code = intro_main return value
    mov eax, 60           ; SYS_exit
    syscall

.hang:
    jmp .hang             ; never reached

section .note.GNU-stack noalloc noexec nowrite progbits
