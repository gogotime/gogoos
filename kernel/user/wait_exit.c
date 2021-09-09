#include "wait_exit.h"
#include "../memory.h"
#include "../global.h"
#include "../../fs/fs.h"
#include "../../lib/debug.h"
#include "../../lib/kernel/stdio.h"
extern List threadAllList;


static void releaseProgramResource(TaskStruct* thread) {
    uint32* pageDirVaddr = thread->pageDir;
    uint16 userPdeNum = 768;
    uint16 pdeIdx = 0;
    uint32 pde = 0;
    uint32* vPdePtr = NULL;

    uint16 userPteNum = 1024;
    uint32 pte = 0;
    uint16 pteIdx = 0;
    uint32* vPtePtr = NULL;

    uint32* firstPteVaddrInPde = NULL;
    uint32 pagePhyAddr = 0;

    while (pdeIdx < userPdeNum) {
        vPdePtr = pageDirVaddr + pdeIdx;
        pde = *vPdePtr;
        if (pde & 0x00000001) {
            firstPteVaddrInPde = getPtePtr(pdeIdx * 0x400000);
            pteIdx = 0;
            while (pteIdx < userPteNum) {
                vPtePtr = firstPteVaddrInPde + pteIdx;
                pte = *vPdePtr;
                if (pte & 0x00000001) {
                    pagePhyAddr = pte & 0xfffff000;
                    freePhyAddr(pagePhyAddr);
                }
                pteIdx++;
            }
            pagePhyAddr = pde & 0xfffff000;
            freePhyAddr(pagePhyAddr);
        }
        pdeIdx++;
    }

    uint32 bmpPgCnt = DIV_ROUND_UP((0xc0000000 - USER_VADDR_START) / PG_SIZE / 8, PG_SIZE);
    freePage(PF_KERNEL, thread->vap.bitMap.startAddr, bmpPgCnt);

    uint8 fdIdx = 3;
    while (fdIdx < MAX_FILE_OPEN_PER_PROC) {
        if (thread->fdTable[fdIdx] != -1) {
            sysClose(fdIdx);
        }
        fdIdx++;
    }
}

static bool findChild(ListElem* elem, int32 ppid) {
    TaskStruct* thread = elemToEntry(TaskStruct, allListTag, elem);
    if (thread->parentPid == ppid) {
        return true;
    }
    return false;
}

static bool findHangingChild(ListElem* elem, int32 ppid) {
    TaskStruct* thread = elemToEntry(TaskStruct, allListTag, elem);
    if (thread->parentPid == ppid && thread->status == TASK_HANGING) {
        return true;
    }
    return false;
}

static bool initAdoptChild(ListElem* elem, int32 pid) {
    TaskStruct* thread = elemToEntry(TaskStruct, allListTag, elem);
    if (thread->parentPid == pid) {
        thread->parentPid = 4;
    }
    return false;
}

int32 sysWait(int32* status) {
    TaskStruct* parentThread = getCurrentThread();
    while (1) {
        ListElem* childElem = listTraversal(&threadAllList, findHangingChild, parentThread->pid);
        if (childElem != NULL) {
            TaskStruct* childThread = elemToEntry(TaskStruct, allListTag, childElem);
            *status = childThread->status;
            uint16 childPid = childThread->pid;
            threadExit(childThread, false);
            return childPid;
        }
        childElem = listTraversal(&threadAllList, findChild, parentThread->pid);
        if (childElem == NULL) {
            return -1;
        } else {
            threadBlock(TASK_WAITING);
        }
    }
}

void sysExit(int32 status) {
    TaskStruct* childThread = getCurrentThread();
    childThread->status = status;
    if (childThread->parentPid == -1) {
        PANIC("sysExit: childThread->parentPid = -1")
    }
    listTraversal(&threadAllList, initAdoptChild, childThread->pid);

    releaseProgramResource(childThread);

    TaskStruct* parentThread = pidToThread(childThread->parentPid);
    if (parentThread->status == TASK_WAITING) {
        threadUnblock(parentThread);
    }
    threadBlock(TASK_HANGING);
}