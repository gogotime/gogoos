# ifndef __KERNEL_THREAD_SYNC_H
# define __KERNEL_THREAD_SYNC_H

#include "../../lib/structure/list.h"
#include "../../lib/stdint.h"
#include "../../lib/debug.h"
#include "thread.h"

typedef struct {
    uint8 value;
    List waiters;
} Semaphore;

typedef struct {
    TaskStruct* holder;
    Semaphore semaphore;
    uint32 holderRepeatTimes;
} Lock;

void semaInit(Semaphore* sema, uint8 value);

void lockInit(Lock* lock);

void semaDown(Semaphore* sema);

void semaUp(Semaphore* sema);

void lockAcquire(Lock* lock);

void lockRelease(Lock* lock);

#endif