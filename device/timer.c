#include "../lib/kernel/io.h"
#include "../lib/kernel/print.h"

#define IRQ0_FREQUENCY 10
#define INPUT_FREQUENCY 1193180
#define COUNTER0_VALUE INPUT_FREQUENCY / IRQ0_FREQUENCY
#define COUNTER0_PORT 0x40
#define COUNTER0_NO 0
#define COUNTER_MODE 2
#define READ_WRITE_LATCH 3
#define PIT_CONTROL_PORT 0x43

static void setFrequency(uint8 counterPort, uint8 counterNo, uint8 rwl, uint8 counterMode, uint16 counterValue) {
    outb(PIT_CONTROL_PORT, (uint8) (counterNo << 6 | rwl << 4 | counterMode << 1));
    outb(counterPort, (uint8) counterValue);
    outb(counterPort, (uint8) (counterValue >> 8));
}

void timerInit() {
    putString("timerInit start\n");
    setFrequency(COUNTER0_PORT, COUNTER0_NO, READ_WRITE_LATCH, COUNTER_MODE, COUNTER0_VALUE);
    putString("timerInit done!\n");
}