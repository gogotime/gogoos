#include "sync.h"
#include "../../lib/kernel/print.h"

void semaInit(Semaphore* sema, uint8 value) {
    sema->value = value;
    listInit(&sema->blockedList);
}

void lockInit(Lock* lock) {
    lock->holder = NULL;
    lock->holderRepeatTimes = 0;
    listInit(&lock->waitingList);
    semaInit(&lock->semaphore, 1);
}

void semaDown(Semaphore* sema) {
    IntrStatus intrStatus = getIntrStatus();
    disableIntr();
    TaskStruct* cur = getCurrentThread();
    while (sema->value == 0) {
        ASSERT(!listElemExist(&sema->blockedList, &cur->generalTag))
        if (listElemExist(&sema->blockedList, &cur->generalTag)) {
            PANIC("semaphoreDown: thread already in blocked list")
        }
        listAppend(&sema->blockedList, &cur->generalTag);
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
    List* list = &sema->blockedList;
    if (!listIsEmpty(list)) {
        TaskStruct* thread = elemToEntry(TaskStruct, generalTag, listPop(list));
        putString("semaUp\n");
        threadUnblock(thread);
    }
    sema->value++;
    ASSERT(sema->value == 1)
    setIntrStatus(intrStatus);
}

void lockLock(Lock* lock) {
    TaskStruct* cur = getCurrentThread();
    if (lock->holder != cur) {
        semaDown(&lock->semaphore);
        lock->holder = cur;
        lock->holderRepeatTimes = 1;
    } else {
        lock->holderRepeatTimes++;
    }
}

void lockUnlock(Lock* lock) {
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

void lockWait(Lock* lock) {
    TaskStruct* cur = getCurrentThread();
    if (lock->holder != cur) {
        PANIC("lockWait:holder isn't current thread")
    }
    ASSERT(!listElemExist(&lock->waitingList, &cur->generalTag))
    if (listElemExist(&lock->waitingList, &cur->generalTag)) {
        PANIC("lockWait: thread already in waiting list")
    }
    listAppend(&lock->waitingList, &cur->generalTag);
    lockUnlock(lock);
    threadBlock(TASK_WAITING);
    lockLock(lock);
}

void lockNotify(Lock* lock) {
    IntrStatus intrStatus = getIntrStatus();
    disableIntr();
    List* list = &lock->waitingList;
    if (!listIsEmpty(list)) {
        TaskStruct* thread = elemToEntry(TaskStruct, generalTag, listPop(list));
        threadUnblock(thread);
    }
    setIntrStatus(intrStatus);
}

void lockNotifyAll(Lock* lock) {
    IntrStatus intrStatus = getIntrStatus();
    disableIntr();
    List* list = &lock->waitingList;
    while (!listIsEmpty(list)) {
        TaskStruct* thread = elemToEntry(TaskStruct, generalTag, listPop(list));
        putString("lockNotifyAll\n");
        threadUnblock(thread);
    }
    setIntrStatus(intrStatus);
}