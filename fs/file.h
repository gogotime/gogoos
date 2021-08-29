#ifndef FS_FILE_H
#define FS_FILE_H

#include "../lib/stdint.h"
#include "inode.h"
#include "dir.h"

#define MAX_FILE_OPEN_ALL 32
#define stdin 0
#define stdout 1
#define stderr 2

typedef enum {
    O_RDONLY,
    O_WRONLY,
    O_RDWR,
    O_CREAT = 4
}OFlags;

typedef struct {
    uint32 fdPos;
    OFlags fdFlag;
    Inode* fdInode;
} File;

typedef enum {
    STDIN,
    STDOUT,
    STDERR
} StdFd;


typedef enum {
    INODE_BITMAP,
    BLOCK_BITMAP
} BitMapType;

int32 getFreeSlotInGlobal();

int32 pcbFdInstall(int32 globalFdIdx);

int32 inodeBitMapAlloc(Partition* part);

int32 blockBitMapAlloc(Partition* part);

void bitMapSync(Partition* part, uint32 bitIdx, BitMapType btype);

int32 fileCreate(Dir* parentDir, char* fileName, OFlags flag);

int32 fileOpen(uint32 ino, OFlags flag);

int32 fileClose(File* file);

int32 fileWrite(File* file, const void* buf, uint32 count);
#endif