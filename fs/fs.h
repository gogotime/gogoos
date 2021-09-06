#ifndef FS_FS_H
#define FS_FS_H

#include "../lib/stdint.h"
#include "inode.h"
#include "dir.h"
#include "file.h"

#define MAX_FILE_PER_PART 4096
#define SECTOR_BYTE_SIZE 512
#define BITS_PER_SECTOR SECTOR_BYTE_SIZE*8
#define BLOCK_BYTE_SIZE SECTOR_BYTE_SIZE
#define MAX_PATH_LEN 512

typedef enum {
    SEEK_SET = 1,
    SEEK_CUR,
    SEEK_END
} SeekFlag;

typedef struct {
    char searchPath[MAX_PATH_LEN];
    Dir* parentDir;
    FileType fileType;
} PathSearchRecord;

typedef struct {
    uint32 ino;
    uint32 size;
    FileType fileType;
} Stat;

void partitionFormat(Partition* partition);

static bool mountPartition(ListElem* elem, int arg);

char* pathParse(char* pathName, char* nameStore);

int32 pathDepthCnt(const char* pathName);

int searchFile(const char* pathName, PathSearchRecord* record);

int32 sysOpen(const char* pathName, OFlags flags);

int32 sysClose(int32 fd);

uint32 sysWrite(int32 fd, const void* buf, uint32 count);

uint32 sysRead(int32 fd, const void* buf, uint32 count);

int32 sysLseek(int32 fd, int32 offset, uint8 seekFlag);

int32 sysUnlink(const char* pathName);

int32 sysMkdir(const char* pathName);

Dir* sysOpenDir(const char* pathName);

int32 sysCloseDir(Dir* dir);

DirEntry* sysReadDir(Dir* dir);

void sysRewindDir(Dir* dir);

int32 sysRmdir(const char* pathName);

char* sysGetCwd(char* buf, uint32 size);

int32 sysChDir(const char* pathName);

int32 sysStat(const char* path, Stat* stat);

void fsInit();

#endif