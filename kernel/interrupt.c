#include "../lib/stdint.h"
#include "../lib/kernel/print.h"
#include "../lib/kernel/io.h"
#include "../lib/kernel/interrupt.h"
#include "../lib/debug.h"
#include "interrupt.h"
#include "global.h"
#include "thread/thread.h"

#define IDT_DESC_CNT 0x30

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



typedef uint32 IntrEntry;
extern IntrEntry intrEntryTable[IDT_DESC_CNT];

char* intrName[IDT_DESC_CNT];
typedef void (*IntrHandler)(uint8 intrNo);
IntrHandler intrHandlerTable[IDT_DESC_CNT];
static GateDesc idt[IDT_DESC_CNT];

extern void setCursor(uint16 pos);

static void defaultIntrHandler(uint8 intrNr) {
    if (intrNr == 0x27 || intrNr == 0x2f) {
        return;
    }
    setCursor(0);
    uint32 cursorPos = 0;
    while (cursorPos < 320) {
        putChar(' ');
        cursorPos++;
    }
    setCursor(0);
    putString("!!!!!!     exception message begin     !!!!!!\n");
    setCursor(88);
    putString("int number : ");
    putUint32Hex(intrNr);
    putString("\n        ");
    putString(intrName[intrNr]);
    if (intrNr == 14) {
        uint32 pageFaultVaddr = 0;
        asm("movl %%cr2,%0":"=r"(pageFaultVaddr));
        putString("\nPage Fault Addr is:");
        putUint32Hex(pageFaultVaddr);
    }
    putString("\n!!!!!!     exception message end       !!!!!!\n");
    while (1);
}

void registerIntrHandler(uint8 intrNo, IntrHandler handler) {
    intrHandlerTable[intrNo] = handler;
}

static void exceptionInit(void) {
    for (int i = 0; i < IDT_DESC_CNT; ++i) {
        intrHandlerTable[i] = defaultIntrHandler;
        intrName[i] = "unknown";
    }
    intrName[0] = "#DE Divide Error";
    intrName[1] = "#DB Debug Exception";
    intrName[2] = "NMI Interrupt";
    intrName[3] = "#BP Breakpoint Exception";
    intrName[4] = "#OF Overflow Exception";
    intrName[5] = "#BR BOUND Range Exceeded Exception";
    intrName[6] = "#UD Invalid Opcode Exception";
    intrName[7] = "#NM Device Not Available Exception";
    intrName[8] = "#DF Double Fault Exception";
    intrName[9] = "Coprocessor Segment Overrun";
    intrName[10] = "#TS Invalid TSS Exception";
    intrName[11] = "#NP Segment Not Present";
    intrName[12] = "#SS Stack Fault Exception";
    intrName[13] = "#GP General Protection Exception";
    intrName[14] = "#PF Page-Fault Exception";
    intrName[15] = "Preserve";
    intrName[16] = "#MF x87 FPU Floating-Point Error";
    intrName[17] = "#AC Alignment Check Exception";
    intrName[18] = "#MC Machine-Check Exception";
    intrName[19] = "#XF SIMD Floating-Point Exception";
}

static void makeIdtDesc(GateDesc* gd, uint8 attr, IntrEntry fn) {
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
//    outb(PIC_M_DATA, 0xfe);
//    outb(PIC_S_DATA, 0xff);

    outb(PIC_M_DATA, 0xfc);
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