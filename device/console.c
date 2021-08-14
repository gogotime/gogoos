#include "../lib/kernel/print.h"
#include "../lib/kernel/asm/put_char.h"
#include "../lib/stdint.h"
#include "../kernel/thread/sync.h"


Lock consoleLock;

void consoleInit() {
    lockInit(&consoleLock);
}

void consolePutString(char* string) {
    lockAcquire(&consoleLock);
    putString(string);
    lockRelease(&consoleLock);
}

void consolePutUint32(uint32 val) {
    lockAcquire(&consoleLock);
    putUint32(val);
    lockRelease(&consoleLock);
}

void consolePutUint32Hex(uint32 val) {
    lockAcquire(&consoleLock);
    putUint32Hex(val);
    lockRelease(&consoleLock);
}


void consolePutChar(char c) {
    lockAcquire(&consoleLock);
    putChar(c);
    lockRelease(&consoleLock);
}