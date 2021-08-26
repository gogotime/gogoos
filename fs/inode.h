#ifndef FS_INODE_H
#define FS_INODE_H

#include "../lib/stdint.h"
#include "../lib/structure/list.h"
struct inode {
    uint32 ino;
    uint32 size; // file size or directory file number
    uint32 openCnt;
    bool writeDeny;

    uint32 block[13];
    ListElem inodeTag;
};

typedef struct inode Inode;
#endif