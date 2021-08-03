#include "../lib/kernel/print.h"
#include "../lib/kernel/io.h"
#include "../device/timer.h"
#include "interrupt.h"
#include "debug.h"
void initAll() {
    putString("init all\n");
    idtInit();
    timerInit();
}

int main() {

    initAll();
//    disableIntr();

//    testPanic("asd");
    ASSERT(0)
//    asm volatile("cli");

    while (1);


    return 0;
}