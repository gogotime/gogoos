#include "../lib/kernel/print.h"
#include "../lib/kernel/io.h"
#include "../lib/kernel/interrupt.h"
#include "../lib/string.h"
#include "../lib/structure/bitmap.h"
#include "../lib/debug.h"
#include "../lib/user/syscall.h"
#include "../lib/stdio.h"

#include "../device/timer.h"
#include "../device/console.h"
#include "../device/keyboard.h"

#include "interrupt.h"
#include "memory.h"
#include "thread/thread.h"
#include "user/tss.h"
#include "user/process.h"
#include "user/syscall.h"

void initAll() {
    putString("init all\n");
    idtInit();
    syscallInit();
    memInit();
    timerInit();
    threadInit();
    consoleInit();
    keyBoardInit();
    tssInit();
    putString("init all done\n");

}

void testThread1(void* arg);

void userProcess(void* arg);

typedef struct {
    uint16 limit;
    uint32 base;
} GDTR;

volatile uint32 cnt=1;

int main() {
    initAll();
    threadStart("thread1",4,testThread1,"a");
    processStart(userProcess, "userproc1");
//    enableIntr();

    while (1) {
//        consolePutString("Main ");
    };
    return 0;
}


void testThread1(void* arg) {
    while (1) {
//        char c = ioQueueGetChar(&keyboardBuf);
        consolePutUint32(cnt);
        consolePutString("\n");
    }
}

void userProcess(void* arg) {
    while (1) {
        printf("%c", 'c');
        printf("%c", '\n');
//        cnt = getPid();
//        ioQueuePutChar(&keyboardBuf, 'c');
//        ioQueuePutChar(&keyboardBuf, ' ');
    }
}