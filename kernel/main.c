#include "../lib/kernel/print.h"
#include "../lib/kernel/io.h"
#include "../lib/kernel/interrupt.h"
#include "../lib/string.h"
#include "../lib/structure/bitmap.h"
#include "../lib/debug.h"
#include "../device/timer.h"
#include "../device/console.h"
#include "../device/keyboard.h"

#include "interrupt.h"
#include "memory.h"
#include "thread/thread.h"

void initAll() {
    putString("init all\n");
    idtInit();
    memInit();
    timerInit();
    threadInit();
    consoleInit();
    keyBoardInit();
}

void testThread1(void* arg);

void testThread2(void* arg);

typedef struct {
    uint16 limit;
    uint32 base;
} GDTR;

int main() {
    initAll();
    GDTR gdtr;
    asm ("sgdt %0;"
    :"=m"(gdtr));
    putUint32(gdtr.limit);
    putChar('\n');
    putUint32Hex(gdtr.base);
    putChar('\n');
    putUint32Hex(gdtr.base);
    putChar('\n');
    putUint32Hex(*(uint32*) 0x904);
    putChar('\n');
//    enableIntr();
    while (1) {
//        consolePutString("Main ");
    };
    return 0;
}


void testThread1(void* arg) {
    while (1) {
        char c = ioQueueGetChar(&keyboardBuf);
        consolePutChar(c);
        consolePutString("\n");
    }
}

void testThread2(void* arg) {
    char* c = (char*) arg;
    while (1) {
        ioQueuePutChar(&keyboardBuf, *c);
    }
}