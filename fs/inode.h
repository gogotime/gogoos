#ifndef FS_INODE_H
#define FS_INODE_H

#include "../lib/stdint.h"
#include "../lib/structure/list.h"
#include "../device/ide.h"


struct inode {
    uint32 ino;
    uint32 size; // file size or directory file number
    uint32 openCnt;
    bool writeDeny;

    uint32 block[13];
    ListElem inodeTag;
};

typedef struct inode Inode;

void inodeSync(Partition* part, Inode* inode, void* ioBuf);

Inode* inodeOpen(Partition* part, uint32 ino);

void inodeClose(Inode* inode);

void inodeInit(uint32 ino, Inode* newInode);

uint32 blockLbaToBitMapIdx(int32 blockLba);
#endif