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

void testThread(void* arg);

int main() {
    initAll();
//    disableIntr();
//    threadStart("ehads", 4, testThread,"AAAA ");
//    threadStart("ehads", 1, testThread,"BBBB ");

    enableIntr();
    while (1){
//        consolePutString("Main ");
    };
    return 0;
}


void testThread(void* arg) {
    char* str = (char*) arg;
    while(1){
//        disableIntr();
//        putString(str);
//        enableIntr();
        consolePutString(str);
    }
}