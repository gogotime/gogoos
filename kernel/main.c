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

int main() {
    initAll();
    TaskStruct* t1 = threadStart("ehads", 4, testThread1, "AAAA ");
    TaskStruct* t2 = threadStart("ehads", 4, testThread2, "B ");
    TaskStruct* t3 = threadStart("ehads", 4, testThread2, "C ");
    putString("TaskStruct Addr:");
    putUint32Hex((uint32) getCurrentThread());
    putString("\n");
    putString("TaskStruct Addr:");
    putUint32Hex((uint32) t1);
    putString("\n");
    putString("TaskStruct Addr:");
    putUint32Hex((uint32) t2);
    putString("\n");
    enableIntr();
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