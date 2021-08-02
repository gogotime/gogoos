#include "../lib/kernel/print.h"
#include "../lib/kernel/io.h"
#include "interrupt.h"
#include "../device/timer.h"

void initAll() {
    putString("init all\n");
    idtInit();
    timerInit();
}

int _start() {

    initAll();
    asm volatile("sti");
//    asm volatile("cli");

    while (1);


    return 0;
}