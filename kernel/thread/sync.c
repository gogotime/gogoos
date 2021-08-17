#include "../../lib/kernel/print.h"
#include "../../lib/debug.h"
#include "thread.h"
#include "../../lib/structure/list.h"

void semaInit(Semaphore* sema, uint8 value) {
    sema->value = value;
    listInit(&sema->blockedList);
    listInit(&sema->waitingList);
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
        ASSERT(!listElemExist(&sema->blockedList, &cur->lockTag))
        if (listElemExist(&sema->blockedList, &cur->lockTag)) {
            PANIC("semaphoreDown: thread already in blocked list")
        }
        listAppend(&sema->blockedList, &cur->lockTag);
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
    while (!listIsEmpty(list)) {
        TaskStruct* thread = elemToEntry(TaskStruct, lockTag, listPop(list));
        if (thread->status == TASK_BLOCKED) {
            threadUnblock(thread);
            break;
        }
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
    IntrStatus intrStatus = getIntrStatus();
    disableIntr();

    TaskStruct* cur = getCurrentThread();
    if (lock->holder != cur) {
        PANIC("lockWait:holder isn't current thread")
    }
    ASSERT(!listElemExist(&lock->semaphore.waitingList, &cur->lockTag))
    if (listElemExist(&lock->semaphore.waitingList, &cur->lockTag)) {
        PANIC("lockWait: thread already in waiting list")
    }
    ASSERT(!listElemExist(&lock->semaphore.blockedList, &cur->lockTag))
    listAppend(&lock->semaphore.waitingList, &cur->lockTag);
    lockUnlock(lock);
    threadBlock(TASK_WAITING);

    setIntrStatus(intrStatus);

    lockLock(lock);
}

void lockNotify(Lock* lock) {
    TaskStruct* cur = getCurrentThread();
    if (lock->holder != cur) {
        PANIC("lockNotify:holder isn't current thread")
    }
    IntrStatus intrStatus = getIntrStatus();
    disableIntr();
    List* list = &lock->semaphore.waitingList;
    if (!listIsEmpty(list)) {
        TaskStruct* thread = elemToEntry(TaskStruct, lockTag, listPop(list));
        threadUnblock(thread);
    }
    setIntrStatus(intrStatus);
}

void lockNotifyAll(Lock* lock) {
    TaskStruct* cur = getCurrentThread();
    if (lock->holder != cur) {
        PANIC("lockNotifyAll:holder isn't current thread")
    }
    IntrStatus intrStatus = getIntrStatus();
    disableIntr();
    List* list = &lock->semaphore.waitingList;
    while (!listIsEmpty(list)) {
        TaskStruct* thread = elemToEntry(TaskStruct, lockTag, listPop(list));
        threadUnblock(thread);
    }
    setIntrStatus(intrStatus);
}