#include "../../lib/string.h"
#include "../../lib/kernel/print.h"
#include "../../lib/debug.h"
#include "thread.h"
#include "../global.h"
#include "../memory.h"

TaskStruct* mainThread;
List threadReadyList;
List threadAllList;

static ListElem* threadTag;

extern void switchTo(TaskStruct* cur, TaskStruct* next);

void schedule() {
    ASSERT(getIntrStatus() == INTR_OFF)
    TaskStruct* cur = getCurrentThread();
    if (cur->status == TASK_RUNNING) {
        ASSERT(!listElemExist(&threadReadyList, &cur->generalTag))
        listAppend(&threadReadyList, &cur->generalTag);
        cur->ticks = cur->priority;
        cur->status = TASK_READY;
    } else {

    }
    ASSERT(!listIsEmpty(&threadReadyList))
    threadTag = NULL;
    threadTag = listPop(&threadReadyList);
    TaskStruct* next = elemToEntry(TaskStruct, generalTag, threadTag);
    next->status = TASK_RUNNING;
    switchTo(cur, next);
}

TaskStruct* getCurrentThread() {
    uint32 esp;
    asm ("mov %%esp,%0;":"=g"(esp));
    return (TaskStruct*) (esp & 0xfffff000);
}

static void kernelThread(ThreadFunc func, void* funcArg) {
    enableIntr();
    func(funcArg);
}

void threadCreate(TaskStruct* pcb, char* name, uint32 priority, ThreadFunc func, void* funcArg) {
    memset(pcb, 0, sizeof(*pcb));
    strcpy(pcb->name, name);

    pcb->status = TASK_READY;
    pcb->selfKnlStack = (uint32*) ((uint32) pcb + PG_SIZE);
    pcb->priority = priority;
    pcb->ticks = priority;
    pcb->elapsedTicks = 0;
    pcb->pageDir = NULL;
    pcb->stackMagicNum = STACK_MAGIC_NUMBER;

    pcb->selfKnlStack -= sizeof(IntrStack);
    pcb->selfKnlStack -= sizeof(ThreadStack);
    ThreadStack* kThreadStack = (ThreadStack*) pcb->selfKnlStack;
    kThreadStack->eip = kernelThread;
    kThreadStack->function = func;
    kThreadStack->funcArg = funcArg;
    kThreadStack->ebp = 0;
    kThreadStack->ebx = 0;
    kThreadStack->esi = 0;
    kThreadStack->edi = 0;
}


TaskStruct* threadStart(char* name, uint32 priority, ThreadFunc function, void* funcArg) {
    TaskStruct* pcb = getKernelPages(1);
    threadCreate(pcb, name, priority, function, funcArg);
    ASSERT(!listElemExist(&threadReadyList, &pcb->generalTag))
    listAppend(&threadReadyList, &pcb->generalTag);
    ASSERT(!listElemExist(&threadAllList, &pcb->allListTag))
    listAppend(&threadAllList, &pcb->allListTag);
    return pcb;
}

void threadBlock(TaskStatus status) {
    ASSERT((status == TASK_BLOCKED) || (status == TASK_HANGING) || (status == TASK_WAITING))
    IntrStatus intrStatus = getIntrStatus();
    disableIntr();
    TaskStruct* cur = getCurrentThread();
    cur->status = status;
    putString("Blocked TaskStruct Addr:");
    putUint32Hex((uint32) cur);
    putString("\n");
    putString("Blocked TaskStruct Status:");
    putUint32((uint32) cur->status);
    putString("\n");
    schedule();
    setIntrStatus(intrStatus);
}

void threadUnblock(TaskStruct* pcb) {
    putString("TaskStruct Addr:");
    putUint32Hex((uint32) pcb);
    putString("\n");
    putString("TaskStruct Status:");
    putUint32((uint32) pcb->status);
    putString("\n");
    ASSERT((pcb->status == TASK_BLOCKED) || (pcb->status == TASK_HANGING) || (pcb->status == TASK_WAITING))
    IntrStatus intrStatus = getIntrStatus();
    disableIntr();
    if (pcb->status != TASK_READY) {
        ASSERT(!listElemExist(&threadReadyList,&pcb->generalTag))
        if (listElemExist(&threadReadyList,&pcb->generalTag)){
            PANIC("threadUnblock: blocked thread in ready list")
        }
        pcb->status = TASK_READY;
        listPush(&threadReadyList, &pcb->generalTag);
    }
    setIntrStatus(intrStatus);
}

static void makeMainThread() {
    mainThread = getCurrentThread();
    memset(mainThread, 0, sizeof(*mainThread));
    strcpy(mainThread->name, "main");

    mainThread->status = TASK_RUNNING;
    mainThread->selfKnlStack = (uint32*) ((uint32) mainThread + PG_SIZE);
    mainThread->priority = 4;
    mainThread->ticks = 4;
    mainThread->elapsedTicks = 0;
    mainThread->pageDir = NULL;
    mainThread->stackMagicNum = STACK_MAGIC_NUMBER;
    ASSERT(!listElemExist(&threadAllList, &mainThread->allListTag))
    listAppend(&threadAllList, &mainThread->allListTag);
}

void threadInit() {
    putString("threadInit start\n");
    listInit(&threadAllList);
    listInit(&threadReadyList);
    makeMainThread();
    putString("threadInit done\n");
}



