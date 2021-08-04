#include "../lib/kernel/print.h"
#include "../lib/kernel/io.h"
#include "../lib/string.h"
#include "../lib/structure/bitmap.h"
#include "../lib/kernel/interrupt.h"
#include "../lib/debug.h"

#include "../device/timer.h"

void initAll() {
    putString("init all\n");
    idtInit();
    timerInit();
}

int main() {
    initAll();
    disableIntr();
    uint32 arr[20];
    BitMap bitmap;
    bitmap.start = (uint8*) arr;
    bitmap.length = 2;
    bitMapInit(&bitmap);
    bitMapAllocAndSet(&bitmap, 9, 1);
    bitMapAllocAndSet(&bitmap, 5, 1);
    bitMapAllocAndSet(&bitmap, 2, 1);
    bitMapPrint(&bitmap);
    bitMapTest(&bitmap, 11);
    putUint32(bitMapTest(&bitmap, 13));
    putUint32(bitMapTest(&bitmap, 14));
    putUint32(bitMapTest(&bitmap, 15));
    putUint32(bitMapTest(&bitmap, 16));

//    bitMapAllocAndSet(&bitmap, 2, 1);
//    bitMapPrint(&bitmap);
    ASSERT(1 == 0)
    while (1);
    return 0;
}