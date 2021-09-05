#include "fork.h"
#include "process.h"
#include "../../lib/string.h"
#include "../../lib/kernel/stdio.h"
#include "../../lib/debug.h"
#include "../../fs/file.h"
#include "../global.h"
#include "../memory.h"

extern uint32 intrExit;
extern File fileTable[MAX_FILE_OPEN_ALL];
extern List threadReadyList;
extern List threadAllList;

static int32 copyPcbVaddrBitMapStack0(TaskStruct* childThread, TaskStruct* parentThread) {
    memcpy(childThread, parentThread, PG_SIZE);
    childThread->pid = forkPid();
    childThread->elapsedTicks = 0;
    childThread->status = TASK_READY;
    childThread->ticks = childThread->priority;
    childThread->parentPid = parentThread->pid;
    childThread->generalTag.prev = childThread->generalTag.next = NULL;
    childThread->allListTag.prev = childThread->allListTag.next = NULL;
    childThread->lockTag.prev = childThread->lockTag.next = NULL;
    memBlockDescInit(childThread->umbdArr);
    uint32 bitMapPgCnt = DIV_ROUND_UP((0xc0000000 - USER_VADDR_START) / PG_SIZE / 8, PG_SIZE);
    void* vaddrBitMap = getKernelPages(bitMapPgCnt);
    if (vaddrBitMap == NULL) {
        return -1;
    }
    memcpy(vaddrBitMap, parentThread->vap.bitMap.startAddr, bitMapPgCnt * PG_SIZE);
    childThread->vap.bitMap.startAddr = vaddrBitMap;
    ASSERT(strlen(childThread->name) < 11);
    strcat(childThread->name, "_fork");
    return 0;
}

static void copyBodyStack3(TaskStruct* childThread, TaskStruct* parentThread, void* bufPage) {
    uint8* vaddrBitMapAddrStart = parentThread->vap.bitMap.startAddr;
    uint32 vaddrBitMapLength = parentThread->vap.bitMap.length;
    uint32 vaddrStart = parentThread->vap.startAddr;
    uint32 idxByte = 0;
    uint32 idxBit = 0;
    uint32 progVaddr = 0;
    while (idxByte < vaddrBitMapLength) {
        if (vaddrBitMapAddrStart[idxByte]!=0) {
            idxBit = 0;
            while (idxBit < 8) {
                if ((0x1 << idxBit) & vaddrBitMapAddrStart[idxByte]) {
                    progVaddr = (idxByte * 8 + idxBit) * PG_SIZE + vaddrStart;
                    memcpy(bufPage, (void*) progVaddr, PG_SIZE);
                    pageDirActivate(childThread);
                    getOnePageWithoutVBM(PF_USER, progVaddr);
                    memcpy((void*) progVaddr, bufPage, PG_SIZE);
                    pageDirActivate(parentThread);
                }
                idxBit++;
            }
        }
        idxByte++;
    }
}

static uint32 buildChildStack(TaskStruct* childThread) {
    IntrStack* intr0Stack = (IntrStack*) ((uint32) childThread + PG_SIZE - sizeof(IntrStack));
    intr0Stack->eax = 0;

    uint32* ret = (uint32*) intr0Stack - 1;
    uint32* esi = (uint32*) intr0Stack - 2;
    uint32* edi = (uint32*) intr0Stack - 3;
    uint32* ebx = (uint32*) intr0Stack - 4;
    uint32* ebp = (uint32*) intr0Stack - 5;
    *ret = (uint32) (&intrExit);
    childThread->selfKnlStack = ebp;
    return 0;
}

static void updateInodeOpenCnt(TaskStruct* thread) {
    int32 localFd = 3;
    int32 globalFd = 0;
    while (localFd < MAX_FILE_OPEN_PER_PROC) {
        globalFd = thread->fdTable[localFd];
        ASSERT(globalFd < MAX_FILE_OPEN_ALL)
        if (globalFd != -1) {
            fileTable[globalFd].fdInode->openCnt++;
        }
        localFd++;
    }
}

static int32 copyProcess(TaskStruct* childThread, TaskStruct* parentThread) {
    void* bufPage = getKernelPages(1);
    if (bufPage == NULL) {
        return -1;
    }
    if (copyPcbVaddrBitMapStack0(childThread, parentThread) == -1) {
        return -1;
    }
    childThread->pageDir = pageDirCreate();
    if (childThread->pageDir == NULL) {
        return -1;
    }
    copyBodyStack3(childThread, parentThread, bufPage);
    buildChildStack(childThread);
    updateInodeOpenCnt(childThread);
    freePage(PF_KERNEL, bufPage, 1);
    return 0;
}

int32 sysFork() {
    TaskStruct* parentThread = getCurrentThread();
    TaskStruct* childThread = getKernelPages(1);
    if (childThread == NULL) {
        return -1;
    }
    ASSERT(getIntrStatus()==INTR_OFF)
    ASSERT(parentThread->pageDir!=NULL)
    if (copyProcess(childThread, parentThread) == -1) {
        return -1;
    }
    ASSERT(!listElemExist(&threadReadyList,&childThread->generalTag))
    listAppend(&threadReadyList, &childThread->generalTag);
    ASSERT(!listElemExist(&threadReadyList,&childThread->allListTag))
    listAppend(&threadAllList, &childThread->allListTag);
    return childThread->pid;
}