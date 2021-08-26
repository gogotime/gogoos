#ifndef FS_FILE_H
#define FS_FILE_H

#include "../lib/stdint.h"
#include "inode.h"

#define MAX_OPEN_FILE 32

typedef struct {
    uint32 fdPos;
    uint32 fdFlag;
    Inode* fdInode;
}File;

typedef enum {
    STDIN,
    STDOUT,
    STDERR
}StdFd;

#define stdin 0
#define stdout 1
#define stderr 2

typedef enum {
    INODE_BITMAP,
    BLOCK_BITMAP
}BitMapType;

#endif