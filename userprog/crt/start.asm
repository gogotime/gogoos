[bits 32]
extern main
section .text
global _start
_start:
    push ecx
    push ebx
    call main