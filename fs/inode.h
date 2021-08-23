#ifndef FS_INODE_H
#define FS_INODE_H

#include "../lib/stdint.h"
#include "../lib/structure/list.h"
struct inode {
    uint32 ino;
    uint32 size;
    uint32 openCnt;
    bool writeDeny;

    uint32 blockIdx[13];
    ListElem inodeTag;
};

typedef struct inode Inode;
#endif