#ifndef DEVICE_IDE_H
#define DEVICE_IDE_H

#include "../fs/super_block.h"
#include "../lib/stdint.h"
#include "../lib/structure/bitmap.h"
#include "../lib/structure/list.h"
#include "../kernel/thread/sync.h"

typedef struct partition Partition;
typedef struct disk Disk;
typedef struct ideChannel IDEChannel;


struct partition {
    uint32 lbaStart;
    uint32 secCnt;
    Disk* disk;
    ListElem partTag;
    SuperBlock * superBlock;
    char name[8];
    BitMap blockBitMap;
    BitMap inodeBitMap;
    List openInodes;
};


struct disk {
    char name[8];
    struct ideChannel* channel;
    uint8 devNo;
    Partition primParts[4];
    Partition logicParts[8];
};


struct ideChannel {
    char name[8];
    bool expectingIntr;
    uint8 intrNo;
    uint16 portBase;
    Lock lock;
    Semaphore diskDone;
    Disk devices[2];
};

void ideInit();

void ideRead(Disk* hd, uint32 lba, void* buf, uint32 secCnt);

void ideWrite(Disk* hd, uint32 lba, void* buf, uint32 secCnt);

#endif