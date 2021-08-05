#include "../lib/kernel/print.h"
#include "../lib/kernel/io.h"
#include "../lib/string.h"
#include "../lib/structure/bitmap.h"
#include "../lib/kernel/interrupt.h"
#include "../lib/debug.h"

#include "../device/timer.h"
#include "memory.h"

void initAll() {
    putString("init all\n");
    idtInit();
    memInit();
    timerInit();
}

int main() {
    initAll();
    disableIntr();
    uint32* p = getKernelPages(3);
    putUint32Hex((uint32)p);
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