
#include "../global.h"
#include "../thread/thread.h"
#include "../../lib/kernel/print.h"
#include "../../lib/string.h"

typedef struct {
    uint16 limitLowWord;
    uint16 baseLowWord;
    uint8 baseMidByte;
    uint8 attrLowByte;
    uint8 LimitHighAttrHighByte;
    uint8 baseHighByte;
} GDTDesc;

typedef struct {
    uint32 backLink;
    uint32* esp0;
    uint32 ss0;
    uint32* esp1;
    uint32 ss1;
    uint32* esp2;
    uint32 ss2;
    uint32 cr3;

    uint32 (* eip)(void);

    uint32 eflags;
    uint32 eax;
    uint32 ecx;
    uint32 edx;
    uint32 ebx;
    uint32 esp;
    uint32 ebp;
    uint32 esi;
    uint32 edi;
    uint32 es;
    uint32 cs;
    uint32 ss;
    uint32 ds;
    uint32 fs;
    uint32 gs;
    uint32 ldt;
    uint32 trace;
    uint32 ioBase;
} TSS;

static TSS tss;

void updateTSSEsp(TaskStruct* ts) {
//    tss.esp0 = ts->selfKnlStack;
    tss.esp0 = (uint32*) ((uint32) ts + PG_SIZE);
}

static GDTDesc makeGDTDesc(uint32 base, uint32 limit, uint8 attrLow, uint8 attrHigh) {
    GDTDesc desc;
    desc.limitLowWord = limit & 0x0000ffff;
    desc.baseLowWord = base & 0x0000ffff;
    desc.baseMidByte = (base & 0x00ff0000) >> 16;
    desc.attrLowByte = attrLow;
    desc.LimitHighAttrHighByte = (((limit & 0x000f0000) >> 16) + attrHigh);
    desc.baseHighByte = (base & 0xff000000) >> 24;
    return desc;
}

void tssInit() {
    putString("tssInit start");
    uint32 tssSize = sizeof(tss);
    putString("tss size:");
    putUint32(tssSize);
    putString("\n");
    memset(&tss, 0, tssSize);
    tss.ss0 = SELECTOR_K_STACK;
    tss.ioBase = tssSize;
    *((GDTDesc*) 0xc0000928) = makeGDTDesc((uint32) &tss, tssSize, TSS_ATTR_LOW, TSS_ATTR_HIGH);
    *((GDTDesc*) 0xc0000930) = makeGDTDesc(0, 0xfffff, GDT_ATTR_LOW_CODE_DPL3, GDT_ATTR_HIGH);
    *((GDTDesc*) 0xc0000938) = makeGDTDesc(0, 0xfffff, GDT_ATTR_LOW_DATA_DPL3, GDT_ATTR_HIGH);
    uint64 gdtOperand = (((uint64) 0xc0000908) << 16 | (8 * 7 - 1));
    asm volatile ("lgdt %0"::"m"(gdtOperand));
    asm volatile ("ltr %w0"::"r"(SELECTOR_TSS));
    putString("tssInit done");
}