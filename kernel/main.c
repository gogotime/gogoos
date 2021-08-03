#include "../lib/kernel/print.h"
#include "../lib/kernel/io.h"
#include "../lib/string.h"
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
    disableIntr();
    while (1);
    return 0;
}