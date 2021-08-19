#include "../lib/kernel/print.h"
#include "../lib/kernel/asm/put_char.h"
#include "../lib/stdint.h"
#include "../kernel/thread/sync.h"
#include <stdarg.h>

Lock consoleLock;

void consoleInit() {
    lockInit(&consoleLock);
}

void consolePutString(char* string) {
    lockLock(&consoleLock);
    putString(string);
    lockUnlock(&consoleLock);
}

void consolePutUint32(uint32 val) {
    lockLock(&consoleLock);
    putUint32(val);
    lockUnlock(&consoleLock);
}

void consolePutUint32Hex(uint32 val) {
    lockLock(&consoleLock);
    putUint32Hex(val);
    lockUnlock(&consoleLock);
}


void consolePutChar(char c) {
    lockLock(&consoleLock);
    putChar(c);
    lockUnlock(&consoleLock);
}
