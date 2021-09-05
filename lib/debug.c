#include "kernel/interrupt.h"
#include "kernel/print.h"

void panicAndSpin1(const char* fileName, uint32 line, const char* funcName, const char* condition) {
    disableIntr();
    putString((char*) fileName);
    sysPutChar(':');
    putUint32(line);
    sysPutChar(':');
    putString((char*) funcName);
    putString(": panic: ");
    putString((char*) condition);
    putString("\n");
    while (1);
}

