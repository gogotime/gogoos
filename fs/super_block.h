#ifndef FS_SUPERBLOCK_H
#define FS_SUPERBLOCK_H

#include "../lib/stdint.h"

struct superBlock {
    uint32 magicNum;
    uint32 secCnt;
    uint32 inodeCnt;
    uint32 partLbaStart;

    uint32 blockBitMapLbaStart;
    uint32 blockBitMapSecCnt;

    uint32 inodeBitMapLbaStart;
    uint32 inodeBitMapSecCnt;

    uint32 inodeTableLbaStart;
    uint32 inodeTableSecCnt;

    uint32 dataLbaStart;
    uint32 rootInodeNo;
    uint32 dirEntrySize;

    uint8 emptyByte[460];
} __attribute__ ((packed));

typedef struct superBlock SuperBlock;
#endif