#ifndef FS_FS_H
#define FS_FS_H

#include "../lib/stdint.h"
#include "inode.h"

#define MAX_FILE_PER_PART 4096
#define SECTOR_BYTE_SIZE 512
#define BITS_PER_SECTOR SECTOR_BYTE_SIZE*8
#define BLOCK_BYTE_SIZE SECTOR_BYTE_SIZE
#define MAX_PATH_LEN 512

typedef enum {
    FT_UNKNOWN,
    FT_REGULAR,
    FT_DIRECTORY
}FileType;

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

static int searchFile(const char* pathName, PathSearchRecord* record);

void fsInit();
#endif