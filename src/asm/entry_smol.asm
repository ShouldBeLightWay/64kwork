;; Entry shim for smol. Reset intro_main's arguments and align its SysV call frame.

extern intro_main

global _start

BITS 64

section .text.startup._start

_start:
    sub rsp, 8
    xor rdi, rdi
    xor rsi, rsi
    call intro_main wrt ..plt

    mov edi, eax
    mov eax, 60
    syscall

.hang:
    jmp .hang

section .note.GNU-stack noalloc noexec nowrite progbits
