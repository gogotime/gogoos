#include "ioqueue.h"

static int32 nextPos(int32 pos) {
    return (pos + 1) % IOQUEUE_BUF_SIZE;
}

void ioQueueInit(IOQueue* queue) {
    lockInit(&queue->lock);
    queue->producer = NULL;
    queue->consumer = NULL;
    queue->head = 0;
    queue->tail = 0;
    queue->isEmpty=true;
    queue->isFull = false;
}

bool ioQueueIsFull(IOQueue* queue) {
    return queue->isFull;
}

bool ioQueueIsEmpty(IOQueue* queue) {
    return queue->isEmpty;
}

void ioQueuePutChar(IOQueue* queue, char elem) {
    lockLock(&queue->lock);
    while (ioQueueIsFull(queue)) {
        lockWait(&queue->lock);
    }
    queue->buf[queue->head] = elem;
    queue->head = nextPos(queue->head);
    queue->isEmpty = false;
    if (queue->head == queue->tail) {
        queue->isFull = true;
    }
    lockNotifyAll(&queue->lock);
    lockUnlock(&queue->lock);
}

char ioQueueGetChar(IOQueue* queue) {
    lockLock(&queue->lock);
    while (ioQueueIsEmpty(queue)) {
        lockWait(&queue->lock);
    }
    char res = queue->buf[queue->tail];
    queue->tail = nextPos(queue->tail);
    queue->isFull = false;
    if (queue->tail == queue->head) {
        queue->isEmpty = true;
    }
    lockNotifyAll(&queue->lock);
    lockUnlock(&queue->lock);
    return res;
}