#include "../lib/stdint.h"
#include "../lib/kernel/print.h"
#include "global.h"

#define IDT_DESC_CNT 0x20

typedef struct {
    uint16 funcOffsetLowWord;
    uint16 selector;
    uint8 dcount;
    uint8 attribute;
    uint16 funcOffsetHighWord;
} GateDesc;

typedef uint32 intrHandler;
extern intrHandler intrEntryTable[IDT_DESC_CNT];

static GateDesc idt[IDT_DESC_CNT];

static void makeIdtDesc(GateDesc *gd, uint8 attr, intrHandler fn) {
    gd->funcOffsetLowWord = fn & 0x0000ffff;
    gd->selector = SELECTOR_K_CODE;
    gd->dcount = 0;
    gd->attribute = attr;
    gd->funcOffsetLowWord = fn & 0xffff0000;
}

static void IdtDescInit(void) {
    for (int i = 0; i < IDT_DESC_CNT; i++) {
        makeIdtDesc(&idt[i], IDT_DESC_ATTR_DPL0, intrEntryTable[i]);
    }
}

void idtInit() {
    putString("idt init start\n");
    IdtDescInit();
//    picInit();
    uint64 idtOperand = ((sizeof(idt) - 1) | ((uint64) idt << 16));
    asm volatile ("lidt %0"::"m"(idtOperand));
    putString("idt init done!\n");

}