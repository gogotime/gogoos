# ifndef __KERNEL_THREAD_H
# define __KERNEL_THREAD_H

#include "../lib/stdint.h"

typedef void (ThreadFunc)(void*);

enum TaskStatus {
    TASK_RUNNING,
    TASK_READY,
    TASK_BLOCKED,
    TASK_WAITING,
    TASK_HANGING,
    TASK_DIED
};

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
    uint32* selfKnlStack;
    enum TaskStatus status;
    uint32 priority;
    char name[16];
    uint32 stackMagicNum;
} TaskStruct;

void threadCreate(TaskStruct* pcb, ThreadFunc func, void* funcArg);

void threadInit(TaskStruct* pcb, char* name, uint32 priority);

TaskStruct* threadStart(char* name, uint32 priority, ThreadFunc function, void* funcArg);




# endif