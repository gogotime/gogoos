#include "../../lib/string.h"
#include "../../lib/kernel/print.h"
#include "../../lib/kernel/stdio.h"
#include "../../lib/debug.h"
#include "thread.h"
#include "../global.h"
#include "../memory.h"
#include "../user/process.h"
#include "../../fs/fs.h"


TaskStruct* mainThread;
TaskStruct* idleThread;

List threadReadyList;
List threadAllList;

static ListElem* threadTag;
Lock pidLock;


extern void switchTo(TaskStruct* cur, TaskStruct* next);

TaskStruct* getCurrentThread() {
    uint32 esp;
    asm ("mov %%esp,%0;":"=g"(esp));
    return (TaskStruct*) (esp & 0xfffff000);
}

static void kernelThread(ThreadFunc func, void* funcArg) {
    enableIntr();
    func(funcArg);
}

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
    if (listIsEmpty(&threadReadyList)) {
        threadUnblock(idleThread);
    }
    threadTag = NULL;
    threadTag = listPop(&threadReadyList);
    TaskStruct* next = elemToEntry(TaskStruct, generalTag, threadTag);
    next->status = TASK_RUNNING;
    processActivate(next);
    switchTo(cur, next);
}

static uint32 allocatePid() {
    static uint32 nextPid = 0;
    lockLock(&pidLock);
    nextPid++;
    lockUnlock(&pidLock);
    return nextPid;
}

void threadCreate(TaskStruct* pcb, char* name, uint32 priority, ThreadFunc func, void* funcArg) {
    memset(pcb, 0, sizeof(*pcb));
    strcpy(pcb->name, name);
    pcb->pid = allocatePid();
    pcb->status = TASK_READY;
    pcb->selfKnlStack = (uint32*) ((uint32) pcb + PG_SIZE);
    pcb->priority = priority;
    pcb->ticks = priority;
    pcb->elapsedTicks = 0;
    pcb->pageDir = NULL;
    pcb->cwdIno = 0;
    pcb->stackMagicNum = STACK_MAGIC_NUMBER;
    pcb->parentPid = -1;
    pcb->fdTable[0] = 0;
    pcb->fdTable[1] = 1;
    pcb->fdTable[2] = 2;

    uint8 idx = 3;
    for (; idx < MAX_FILE_OPEN_PER_PROC; idx++) {
        pcb->fdTable[idx] = -1;
    }

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
    ASSERT(!listElemExist(&threadReadyList, &cur->generalTag))
    cur->status = status;
    schedule();

    setIntrStatus(intrStatus);
}

void threadUnblock(TaskStruct* pcb) {
    ASSERT(!listElemExist(&threadReadyList, &pcb->generalTag))
    ASSERT((pcb->status == TASK_BLOCKED) || (pcb->status == TASK_HANGING) || (pcb->status == TASK_WAITING))
    IntrStatus intrStatus = getIntrStatus();
    disableIntr();
    if (pcb->status != TASK_READY) {
        ASSERT(!listElemExist(&threadReadyList, &pcb->generalTag))
        if (listElemExist(&threadReadyList, &pcb->generalTag)) {
            PANIC("threadUnblock: blocked thread in ready list")
        }
        pcb->status = TASK_READY;
        listPush(&threadReadyList, &pcb->generalTag);
    }
    setIntrStatus(intrStatus);
}

void threadYield() {
    IntrStatus intrStatus = getIntrStatus();
    disableIntr();
    TaskStruct* cur = getCurrentThread();
    ASSERT(!listElemExist(&threadReadyList, &cur->generalTag))
    listAppend(&threadReadyList, &cur->generalTag);
    cur->status = TASK_READY;
    schedule();
    setIntrStatus(intrStatus);
}

static void idleThreadFunc(void* unusedArg) {
    while (1) {
        threadBlock(TASK_BLOCKED);
//        putString("IDLE\n");
        asm volatile ("sti;hlt;":: :"memory");
    }
}

static void makeMainThread() {
    mainThread = getCurrentThread();
    memset(mainThread, 0, sizeof(*mainThread));
    strcpy(mainThread->name, "main");
    mainThread->pid = allocatePid();
    mainThread->status = TASK_RUNNING;
    mainThread->selfKnlStack = (uint32*) ((uint32) mainThread + PG_SIZE);
    mainThread->priority = 4;
    mainThread->ticks = 4;
    mainThread->elapsedTicks = 0;
    mainThread->pageDir = NULL;
    mainThread->cwdIno = 0;
    mainThread->parentPid = -1;
    mainThread->stackMagicNum = STACK_MAGIC_NUMBER;

    mainThread->fdTable[0] = 0;
    mainThread->fdTable[1] = 1;
    mainThread->fdTable[2] = 2;

    uint8 idx = 3;
    for (; idx < MAX_FILE_OPEN_PER_PROC; idx++) {
        mainThread->fdTable[idx] = -1;
    }

    ASSERT(!listElemExist(&threadAllList, &mainThread->allListTag))
    listAppend(&threadAllList, &mainThread->allListTag);
}

uint32 sysGetPid() {
    return getCurrentThread()->pid;
}

int32 forkPid() {
    return allocatePid();
}

static bool threadPrintFunc(ListElem* elem, int unusedArg) {
    TaskStruct* thread = elemToEntry(TaskStruct, allListTag, elem);
    char buf[16] = {' '};
    memset(buf, ' ', 11);
    sprintk(buf, "%d", thread->pid);
    sysWrite(1, buf, 11);

    memset(buf, ' ', 16);
    if (thread->parentPid == -1) {
        sysWrite(1, "NULL       ", 11);
    }else{
        sprintk(buf, "%d", thread->parentPid);
        sysWrite(1, buf, 11);
    }


    memset(buf, ' ', 16);
    sprintk(buf, "%s", thread->name);
    for (int32 i = 0; i < 16; i++) {
        if (buf[i] == 0) {
            buf[i] = ' ';
        }
    }
    sysWrite(1, buf, 16);

    memset(buf, ' ', 11);
    sprintk(buf, "%d", thread->priority);
    sysWrite(1, buf, 11);

    memset(buf, ' ', 11);
    sprintk(buf, "%d", thread->ticks);
    sysWrite(1, buf, 11);

    switch (thread->status) {
        case TASK_RUNNING:
            sysWrite(1, "RUNNING   ", 11);
            break;
        case TASK_READY:
            sysWrite(1, "READY     ", 11);
            break;
        case TASK_BLOCKED:
            sysWrite(1, "BLOCKED   ", 11);
            break;
        case TASK_WAITING:
            sysWrite(1, "WAITING   ", 11);
            break;
        case TASK_HANGING:
            sysWrite(1, "HANGING   ", 11);
            break;
        case TASK_DIED:
            sysWrite(1, "DIED      ", 11);
            break;
    }
    sysWrite(1, "\n", 1);
    return false;
}

void sysPs() {
    char* title = "PID        PPID       NAME            PRIORITY   TICKS      STATUS\n";
    sysWrite(1, title, strlen(title));
    listTraversal(&threadAllList, threadPrintFunc, 0);
}

void threadInit() {
    putString("threadInit start\n");
    listInit(&threadAllList);
    listInit(&threadReadyList);
    lockInit(&pidLock);
    makeMainThread();
    idleThread = threadStart("idle", 1, idleThreadFunc, NULL);
    putString("threadInit done\n");
}



