#ifndef FS_FILE_H
#define FS_FILE_H

#include "../lib/stdint.h"
#include "inode.h"
#include "dir.h"
#define MAX_FILE_OPEN_ALL 32

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

int32 getFreeSlotInGlobal();

int32 pcbFdInstall(int32 globalFdIdx);

int32 inodeBitMapAlloc(Partition* part);

int32 blockBitMapAlloc(Partition* part);

void bitMapSync(Partition* part, uint32 bitIdx, BitMapType btype);

int32 fileCreate(Dir* parentDir, char* fileName, uint8 flag);
#endif