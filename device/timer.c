#include "../lib/kernel/io.h"
#include "../lib/kernel/print.h"
#include "../kernel/interrupt.h"
#include "../lib/debug.h"
#include "../kernel/thread/thread.h"
#include "../kernel/global.h"

#define IRQ0_FREQUENCY 100
#define INPUT_FREQUENCY 1193180
#define MILLISECONDS_PER_INTR (1000/IRQ0_FREQUENCY)
#define COUNTER0_VALUE (uint16)(INPUT_FREQUENCY / IRQ0_FREQUENCY)
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

uint32 ticks;

static void intrTimerHandler(uint8 intrNr) {
    TaskStruct* curThread = getCurrentThread();
    ASSERT(curThread->stackMagicNum == STACK_MAGIC_NUMBER)
    curThread->elapsedTicks++;
    ticks++;
    if (curThread->ticks == 0) {
        schedule();
    } else {
        curThread->ticks--;
    }
}

void timerInit() {
    putString("timerInit start\n");
    setFrequency(COUNTER0_PORT, COUNTER0_NO, READ_WRITE_LATCH, COUNTER_MODE, COUNTER0_VALUE);
    registerIntrHandler(0x20, intrTimerHandler);
    putString("timerInit done!\n");
}

static void ticksToSleep(uint32 sleepTicks) {
    uint32 startTick = ticks;
    while (ticks - startTick < sleepTicks) {
        threadYield();
    }
}

void sleepMs(uint32 milliSeconds) {
    uint32 sleepTicks = DIV_ROUND_UP(milliSeconds, MILLISECONDS_PER_INTR);
    ticksToSleep(sleepTicks);
}