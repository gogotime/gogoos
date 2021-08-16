[bits 32]
%define DO_NOTHING nop
%macro PUSH_ZERO 0
    push 0
%endmacro

extern intrHandlerTable

%macro VECTOR 2
section .text
intr%1Entry:
    %2
    push ds
    push es
    push fs
    push gs
    pushad
; send EOI
    mov al,0x20
    out 0xa0,al
    out 0x20,al
    push %1
    call [intrHandlerTable+%1*4]
    jmp intr_exit

section .data
    dd intr%1Entry

%endmacro
global intrExit
section .text
intrExit:
intr_exit:
    add esp,4
    popad
    pop gs
    pop fs
    pop es
    pop ds

    add esp,4
    iret

section .data

global intrEntryTable
intrEntryTable:
    VECTOR 0x00,PUSH_ZERO
    VECTOR 0x01,PUSH_ZERO
    VECTOR 0x02,PUSH_ZERO
    VECTOR 0x03,PUSH_ZERO
    VECTOR 0x04,PUSH_ZERO
    VECTOR 0x05,PUSH_ZERO
    VECTOR 0x06,PUSH_ZERO
    VECTOR 0x07,PUSH_ZERO
    VECTOR 0x08,PUSH_ZERO
    VECTOR 0x09,PUSH_ZERO
    VECTOR 0x0a,PUSH_ZERO
    VECTOR 0x0b,PUSH_ZERO
    VECTOR 0x0c,PUSH_ZERO
    VECTOR 0x0d,PUSH_ZERO
    VECTOR 0x0e,PUSH_ZERO
    VECTOR 0x0f,PUSH_ZERO

    VECTOR 0x10,PUSH_ZERO
    VECTOR 0x11,PUSH_ZERO
    VECTOR 0x12,PUSH_ZERO
    VECTOR 0x13,PUSH_ZERO
    VECTOR 0x14,PUSH_ZERO
    VECTOR 0x15,PUSH_ZERO
    VECTOR 0x16,PUSH_ZERO
    VECTOR 0x17,PUSH_ZERO
    VECTOR 0x18,PUSH_ZERO
    VECTOR 0x19,PUSH_ZERO
    VECTOR 0x1a,PUSH_ZERO
    VECTOR 0x1b,PUSH_ZERO
    VECTOR 0x1c,PUSH_ZERO
    VECTOR 0x1d,PUSH_ZERO
    VECTOR 0x1e,DO_NOTHING
    VECTOR 0x1f,PUSH_ZERO

    VECTOR 0x20,PUSH_ZERO ;clock
    VECTOR 0x21,PUSH_ZERO ;keyboard
    VECTOR 0x22,PUSH_ZERO
    VECTOR 0x23,PUSH_ZERO
    VECTOR 0x24,PUSH_ZERO
    VECTOR 0x25,PUSH_ZERO
    VECTOR 0x26,PUSH_ZERO
    VECTOR 0x27,PUSH_ZERO
    VECTOR 0x28,PUSH_ZERO
    VECTOR 0x29,PUSH_ZERO
    VECTOR 0x2a,PUSH_ZERO
    VECTOR 0x2b,PUSH_ZERO
    VECTOR 0x2c,PUSH_ZERO
    VECTOR 0x2d,PUSH_ZERO
    VECTOR 0x2e,PUSH_ZERO ;disk
    VECTOR 0x2f,PUSH_ZERO