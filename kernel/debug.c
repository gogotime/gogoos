#include "interrupt.h"
#include "../lib/kernel/print.h"

void panicAndSpin1(const char* fileName, uint32 line, const char* funcName) {
    disableIntr();
    putString((char*) fileName);
    putChar(':');
    putUint32(line);
    putChar(':');
    putString((char*) funcName);
    putString(": panic: ");
}

void panicAndSpin2(const char* condition) {
    putString((char*) condition);
    putString("\n");
    while (1);
}
