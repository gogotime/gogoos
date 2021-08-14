# ifndef __KERNEL_THREAD_SYNC_H
# define __KERNEL_THREAD_SYNC_H

#include "../../lib/structure/list.h"
#include "../../lib/stdint.h"
#include "../../lib/debug.h"
#include "thread.h"

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

void semaInit(Semaphore* sema, uint8 value);

void lockInit(Lock* lock);

void semaDown(Semaphore* sema);

void semaUp(Semaphore* sema);

void lockLock(Lock* lock);

void lockUnlock(Lock* lock);

void lockWait(Lock* lock);

void lockNotify(Lock* lock);

void lockNotifyAll(Lock* lock);

#endif