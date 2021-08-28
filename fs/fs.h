#ifndef FS_FS_H
#define FS_FS_H

#include "../lib/stdint.h"
#include "inode.h"
#include "dir.h"

#define MAX_FILE_PER_PART 4096
#define SECTOR_BYTE_SIZE 512
#define BITS_PER_SECTOR SECTOR_BYTE_SIZE*8
#define BLOCK_BYTE_SIZE SECTOR_BYTE_SIZE
#define MAX_PATH_LEN 512


typedef enum {
    O_RDONLY,
    O_WRONLY,
    O_RDWR,
    O_CREAT = 4
}Oflags;

typedef struct {
    char searchPath[MAX_PATH_LEN];
    Dir* parentDir;
    FileType fileType;
}PathSearchRecord;

static void partitionFormat(Partition* partition);

static bool mountPartition(ListElem* elem, int arg);

static char* pathParse(char* pathName, char* nameStore);

int32 pathDepthCnt(const char* pathName);

int searchFile(const char* pathName, PathSearchRecord* record);

int32 sysOpen(const char* pathName, uint8 flags);

void fsInit();
#endif