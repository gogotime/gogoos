# ifndef __LIB_STRUCTURE_IOQUEUE_H
# define __LIB_STRUCTURE_IOQUEUE_H

#include "../stdint.h"
#include "../../kernel/thread/sync.h"
#include "../../kernel/thread/thread.h"

#define IOQUEUE_BUF_SIZE 10

typedef struct {
    Lock lock;
    TaskStruct* producer;
    TaskStruct* consumer;
    char buf[IOQUEUE_BUF_SIZE];
    int32 head;
    int32 tail;
    bool isFull;
    bool isEmpty;
} IOQueue;


void ioQueueInit(IOQueue* queue);

bool ioQueueIsFull(IOQueue* queue);

bool ioQueueIsEmpty(IOQueue* queue);

void ioQueuePutChar(IOQueue* queue, char elem);

char ioQueueGetChar(IOQueue* queue);


# endif