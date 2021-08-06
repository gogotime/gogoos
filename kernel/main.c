#include "../lib/kernel/print.h"
#include "../lib/kernel/io.h"
#include "../lib/string.h"
#include "../lib/structure/bitmap.h"
#include "../lib/debug.h"
#include "../device/timer.h"

#include "interrupt.h"
#include "memory.h"
#include "thread.h"

void initAll() {
    putString("init all\n");
    idtInit();
    memInit();
    timerInit();
//    enableIntr();
}

void testThread(void* arg);

int main() {
    initAll();
//    disableIntr();
    threadStart("ehads", 0, testThread,"hello");
//    threadStart("ehads", 0);
//    putUint32Hex(*getPdePtr(0xc0000000));
//    putString("\n");
//    putUint32Hex(*getPdePtr(0x00000000));
//    putString("\n");
//    putUint32Hex(*getPtePtr(0xc0000000));
//    bitMapAllocAndSet(&bitmap, 2, 1);
//    bitMapPrint(&bitmap);
//    ASSERT(1 == 0)
    while (1);
    return 0;
}


void testThread(void* arg) {
    char* str = (char*) arg;
    while(1){
        putString(str);
    }
}