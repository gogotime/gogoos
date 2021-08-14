#include "interrupt.h"

#define GET_EFLAGS(EFLAGS_VAR) asm volatile ("pushfl;popl %0":"=g"(EFLAGS_VAR))
#define EFLAGS_IF 0x00000200

void enableIntr() {
    asm volatile("sti");
}

void disableIntr() {
    asm volatile("cli");
}

void setIntrStatus(IntrStatus status) {
    status == INTR_ON ? enableIntr() : disableIntr();
}

IntrStatus getIntrStatus() {
    uint32 eflags;
    GET_EFLAGS(eflags);
    return (eflags&EFLAGS_IF) ? INTR_ON : INTR_OFF;
}