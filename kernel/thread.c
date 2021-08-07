#include "../lib/string.h"
#include "../lib/kernel/print.h"
#include "../lib/debug.h"
#include "thread.h"
#include "global.h"
#include "memory.h"

TaskStruct * mainThread;
List threadReadyList;
List threadAllList;

static ListElem * threadTag;

extern void switchTo(TaskStruct* cur,TaskStruct* next);

TaskStruct * getCurrentThread(){
    uint32 esp;
    asm ("mov %%esp,%0",:"g"(esp));
    return (TaskStruct*) (esp & 0xfffff000);
}

static void kernelThread(ThreadFunc func, void* funcArg) {
    enableIntr();
    func(funcArg);
}

void threadCreate(TaskStruct* pcb, ThreadFunc func, void* funcArg) {
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

void threadInit(TaskStruct* pcb, char* name, uint32 priority) {
    memset(pcb, 0, sizeof(*pcb));
    strcpy(pcb->name, name);

    if (pcb == mainThread) {
        pcb->status = TASK_RUNNING;
    }else{
        pcb->status = TASK_READY;
    }
    pcb->selfKnlStack = (uint32*) ((uint32) pcb + PG_SIZE);
    pcb->priority = priority;
    pcb->ticks = priority;
    pcb->elapsedTicks = 0;
    pcb->pageDir = NULL;
    pcb->stackMagicNum = STACK_MAGIC_NUMBER;
}

TaskStruct* threadStart(char* name, uint32 priority, ThreadFunc function, void* funcArg) {
    TaskStruct* pcb = getKernelPages(1);
    putUint32Hex((uint32)pcb);
    threadInit(pcb, name, priority);
    threadCreate(pcb, function, funcArg);
    ASSERT(!listElemExist(&threadReadyList,&pcb->generalTag))
    listAppend(&threadReadyList, &pcb->generalTag);
    ASSERT(!listElemExist(&threadAllList,&pcb->allListTag))
    listAppend(&threadAllList, &pcb->allListTag);
    return pcb;
    asm volatile(
    "movl %0,%%esp;"
    "pop %%ebp;"
    "pop %%ebx;"
    "pop %%edi;"
    "pop %%esi;"
    "ret;"
    :
    :"g"(pcb->selfKnlStack)
    :"memory"
    );
    return pcb;
}

static void makeMainThread(){
    mainThread = getCurrentThread();
    threadInit(mainThread, "main", 31);
    ASSERT(!listElemExist(&threadAllList,&pcb->allListTag))
    listAppend(&threadAllList, &pcb->allListTag);
}

