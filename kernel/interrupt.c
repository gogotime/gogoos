#include "../lib/stdint.h"
#include "../lib/kernel/print.h"
#include "global.h"
#include "../lib/kernel/io.h"
#include "interrupt.h"


#define IDT_DESC_CNT 0x21

#define PIC_M_CTRL 0x20
#define PIC_M_DATA 0x21
#define PIC_S_CTRL 0xa0
#define PIC_S_DATA 0xa1


typedef struct {
    uint16 funcOffsetLowWord;
    uint16 selector;
    uint8 dcount;
    uint8 attribute;
    uint16 funcOffsetHighWord;
} GateDesc;

typedef void* intrHandler;
typedef uint32 intrEntry;
extern intrEntry intrEntryTable[IDT_DESC_CNT];

char* intrName[IDT_DESC_CNT]; //no use
intrHandler intrHandlerTable[IDT_DESC_CNT];
static GateDesc idt[IDT_DESC_CNT];





static void defaultIntrHandler(uint8 intrNr) {
    if (intrNr == 0x27 || intrNr == 0x2f) {
        return;
    }
    putString("int number : ");
    putUint32Hex(intrNr);
    putChar('\n');
}

static void intr0x20Handler(uint8 intrNr) {
    static int a = 0;
    putString("int number : ");
    putUint32Hex(intrNr);
    putChar(' ');
    putUint32Hex(a++);
    putChar('\n');
}

static void exceptionInit(void) {
    for (int i = 0; i < IDT_DESC_CNT; ++i) {
        intrHandlerTable[i] = defaultIntrHandler;
        intrName[i] = "unknown";
    }
    intrHandlerTable[0x20] = intr0x20Handler;
}

static void makeIdtDesc(GateDesc* gd, uint8 attr, intrEntry fn) {
    gd->funcOffsetLowWord = (uint16) fn;
    gd->selector = SELECTOR_K_CODE;
    gd->dcount = 0;
    gd->attribute = attr;
    gd->funcOffsetHighWord = (uint16) (fn >> 16);
}

static void idtDescInit(void) {
    for (int i = 0; i < IDT_DESC_CNT; i++) {
        makeIdtDesc(&idt[i], IDT_DESC_ATTR_DPL0, intrEntryTable[i]);
    }
}

static void picInit(void) {
    // 8259A Master
    outb(PIC_M_CTRL, 0x11);
    outb(PIC_M_DATA, 0x20);
    outb(PIC_M_DATA, 0x04);
    outb(PIC_M_DATA, 0x01);
    // 8259A Slave
    outb(PIC_S_CTRL, 0x11);
    outb(PIC_S_DATA, 0x28);
    outb(PIC_S_DATA, 0x02);
    outb(PIC_S_DATA, 0x01);
    // Open IR0
    outb(PIC_M_DATA, 0xfe);
    outb(PIC_S_DATA, 0xff);
    putString("pic init done!\n");
}


void idtInit() {
    putString("idt init start\n");
    exceptionInit();
    idtDescInit();
    picInit();
    uint64 idtOperand = ((sizeof(idt) - 1) | ((uint64) idt << 16));
    asm volatile ("lidt %0"::"m"(idtOperand));
    putString("idt init done!\n");

}