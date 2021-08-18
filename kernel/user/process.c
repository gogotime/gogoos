#include "../global.h"
#include "../thread/thread.h"
#include "../memory.h"
#include "../../device/console.h"
#include "../../lib/string.h"
#include "../../lib/debug.h"
#include "process.h"
#include "tss.h"

#define USER_STACK_VADDR (0xc0000000 - 0x1000)
#define DIV_ROUND_UP(X, STEP) ((X+STEP-1)/(STEP))
#define USER_VADDR_START 0x0

extern void intrExit();

extern List threadReadyList;
extern List threadAllList;

void processExecute(void* fileName) {
    void* func = fileName;
    TaskStruct* cur = getCurrentThread();
    cur->selfKnlStack += sizeof(ThreadStack);
    IntrStack* procStack = (IntrStack*) cur->selfKnlStack;
    procStack->edi = procStack->esi = procStack->ebp = procStack->espDummy = 0;
    procStack->ebx = procStack->edx = procStack->ecx = procStack->eax = 0;
    procStack->gs = 0;
    procStack->ds = procStack->es = procStack->fs = SELECTOR_U_DATA;
    procStack->eip = func;
    procStack->cs = SELECTOR_U_CODE;
    procStack->eflags = (EFLAGS_IOPL_0 | EFLAGS_MBS | EFLAGS_IF_1);
    procStack->esp = (void*) ((uint32) getOnePage(PF_USER, USER_STACK_VADDR) + PG_SIZE);
    ASSERT(procStack->esp != NULL)
    procStack->ss = SELECTOR_U_DATA;
    asm volatile ("movl %0,%%esp;"
                  "jmp intrExit;"
    :
    :"g"(procStack)
    :"memory");
}

void pageDirActivate(TaskStruct* ts) {
    uint32 pageDirPhyAddr = PAGE_DIR_TABLE_POS;
    if (ts->pageDir != NULL) {
        pageDirPhyAddr = addrV2P((uint32) ts->pageDir);
    }
    asm volatile ("mov %0,%%cr3"::"r"(pageDirPhyAddr):"memory");
}

void processActivate(TaskStruct* ts) {
    ASSERT(ts != NULL)
    pageDirActivate(ts);
    if (ts->pageDir != NULL) {
        updateTSSEsp(ts);
    }
}

uint32* pageDirCreate(void) {
    uint32* pageDirVaddr = getKernelPages(1);
    if (pageDirVaddr == NULL) {
        consolePutString("pageDirCreate: getKernelPages failed\n");
    }
    memcpy((uint32*) ((uint32) pageDirVaddr + 0x300 * 4), (uint32*) (0xfffff000 + 0x300 * 4), 1024);
    uint32 pageDirRaddr = addrV2P((uint32) pageDirVaddr);
    pageDirVaddr[1023] = pageDirRaddr | PG_US_U | PG_RW_W | PG_P_1;
    return pageDirVaddr;
}

void userVaddrBitMapInit(TaskStruct* userTs) {
    userTs->vap.startAddr = USER_VADDR_START;
    uint32 bmpPgCnt = DIV_ROUND_UP((0xc0000000 - USER_VADDR_START) / PG_SIZE / 8, PG_SIZE);
    consolePutUint32(bmpPgCnt);
    userTs->vap.bitMap.startAddr = getKernelPages(bmpPgCnt);
    userTs->vap.bitMap.length = (0xc0000000 - USER_VADDR_START) / PG_SIZE / 8;
    bitMapInit(&userTs->vap.bitMap);
    consolePutUint32Hex(userTs->vap.bitMap.length);
}

static void userThread(ThreadFunc func, void* funcArg) {
    processExecute(func);
}

void processStart(void* fileName, char* name) {
    TaskStruct* ts = getKernelPages(1);
    threadCreate(ts, name, 12, fileName, "111");
    userVaddrBitMapInit(ts);
    ts->pageDir = pageDirCreate();
    ThreadStack* stack = (ThreadStack*) ts->selfKnlStack;
    stack->eip = userThread;
    stack->function = fileName;
    stack->funcArg = 0;

    IntrStatus intrStatus = getIntrStatus();
    disableIntr();

    ASSERT(!listElemExist(&threadReadyList, &ts->generalTag))
    ASSERT(!listElemExist(&threadAllList, &ts->allListTag))
    listAppend(&threadReadyList, &ts->generalTag);
    listAppend(&threadAllList, &ts->allListTag);

    setIntrStatus(intrStatus);
}
