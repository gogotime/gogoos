#include "sync.h"
#include "../../lib/kernel/print.h"

void semaInit(Semaphore* sema, uint8 value) {
    sema->value = value;
    listInit(&sema->waiters);
}

void lockInit(Lock* lock) {
    lock->holder = NULL;
    lock->holderRepeatTimes = 0;
    semaInit(&lock->semaphore, 1);
}

void semaDown(Semaphore* sema) {
    IntrStatus intrStatus = getIntrStatus();
    disableIntr();
    TaskStruct* cur = getCurrentThread();
    while (sema->value == 0) {
        ASSERT(!listElemExist(&sema->waiters, &cur->generalTag))
        if (listElemExist(&sema->waiters, &cur->generalTag)) {
            PANIC("semaphoreDown: blocked thread in waiters list")
        }
        listAppend(&sema->waiters, &cur->generalTag);
        threadBlock(TASK_BLOCKED);
    }
    sema->value--;
    ASSERT(sema->value == 0)
    setIntrStatus(intrStatus);
}

void semaUp(Semaphore* sema) {
    IntrStatus intrStatus = getIntrStatus();
    disableIntr();
    ASSERT(sema->value == 0)
    if (!listIsEmpty(&sema->waiters)) {
        TaskStruct* blockedThread = elemToEntry(TaskStruct, generalTag, listPop(&sema->waiters));
        threadUnblock(blockedThread);
    }
    sema->value++;
    ASSERT(sema->value == 1)
    setIntrStatus(intrStatus);
}

void lockAcquire(Lock* lock) {
    TaskStruct* cur = getCurrentThread();
    if (lock->holder != cur) {
        semaDown(&lock->semaphore);
        lock->holder = cur;
        lock->holderRepeatTimes = 1;
    } else {
        lock->holderRepeatTimes++;
    }
}

void lockRelease(Lock* lock) {
    ASSERT(lock->holder == getCurrentThread())
    if (lock->holderRepeatTimes > 1) {
        lock->holderRepeatTimes--;
        return;
    }
    ASSERT(lock->holderRepeatTimes == 1)
    lock->holder = NULL;
    lock->holderRepeatTimes = 0;
    semaUp(&lock->semaphore);
}