# ifndef __KERNEL_THREAD_THREAD_H
# define __KERNEL_THREAD_THREAD_H

#include "../../lib/stdint.h"
#include "../../lib/structure/list.h"
#include "../../lib/structure/bitmap.h"


#define STACK_MAGIC_NUMBER 0x20212021
#define MEM_BLOCK_DESC_CNT 7
#define MAX_FILE_OPEN_PER_PROC 7

typedef struct {
    BitMap bitMap;
    uint32 startAddr;
} VirtualAddrPool;

typedef void (ThreadFunc)(void*);

typedef enum {
    TASK_RUNNING,
    TASK_READY,
    TASK_BLOCKED,
    TASK_WAITING,
    TASK_HANGING,
    TASK_DIED
} TaskStatus;

typedef struct {
    uint32 vecNo;
    uint32 edi;
    uint32 esi;
    uint32 ebp;
    uint32 espDummy;
    uint32 ebx;
    uint32 edx;
    uint32 ecx;
    uint32 eax;
    uint32 gs;
    uint32 fs;
    uint32 es;
    uint32 ds;

    uint32 errCode;

    void (* eip)(void);

    uint32 cs;
    uint32 eflags;
    void* esp;
    uint32 ss;

} IntrStack;

typedef struct {
    uint32 ebp;
    uint32 ebx;
    uint32 edi;
    uint32 esi;

    void (* eip)(ThreadFunc func, void* funcArg);

    void(* unusedRetAddr);

    ThreadFunc* function;
    void* funcArg;

} ThreadStack;

typedef struct {
    ListElem freeElem;
} MemBlock;

typedef struct {
    uint32 blockSize;
    uint32 blocksPerArena;
    List freeList;
} MemBlockDesc;

typedef struct {
    uint32* selfKnlStack;
    TaskStatus status;
    uint32 pid;
    uint32 priority;
    char name[16];
    uint32 ticks;
    uint32 elapsedTicks;
    int32 fdTable[MAX_FILE_OPEN_PER_PROC];
    ListElem generalTag;
    ListElem lockTag;
    ListElem allListTag;
    uint32* pageDir;
    MemBlockDesc umbdArr[MEM_BLOCK_DESC_CNT];
    VirtualAddrPool vap;
    uint32 stackMagicNum;
} TaskStruct;

typedef struct {
    uint8 value;
    List blockedList;
    List waitingList;
} Semaphore;

typedef struct {
    TaskStruct* holder;
    Semaphore semaphore;
    uint32 holderRepeatTimes;
} Lock;

TaskStruct* getCurrentThread();

TaskStruct* threadStart(char* name, uint32 priority, ThreadFunc function, void* funcArg);

void threadCreate(TaskStruct* pcb, char* name, uint32 priority, ThreadFunc func, void* funcArg);

void threadInit();

void schedule();

void threadUnblock(TaskStruct* pcb);

void threadBlock(TaskStatus status);

void threadYield();

# endif