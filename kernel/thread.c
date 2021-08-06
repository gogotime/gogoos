#include "../lib/string.h"
#include "../lib/kernel/print.h"
#include "thread.h"
#include "global.h"
#include "memory.h"
static void kernelThread(ThreadFunc func, void* funcArg) {
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
    pcb->status = TASK_RUNNING;
    pcb->priority = priority;
    pcb->selfKnlStack = (uint32*) ((uint32) pcb + PG_SIZE);
    pcb->stackMagicNum = 0x20202021;
}

TaskStruct* threadStart(char* name, uint32 priority, ThreadFunc function, void* funcArg) {
    TaskStruct* pcb = getKernelPages(1);
    putUint32Hex((uint32)pcb);
    threadInit(pcb, name, priority);
    threadCreate(pcb, function, funcArg);
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

