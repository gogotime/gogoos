#ifndef FS_DIR_H
#define FS_DIR_H

#include "../lib/stdint.h"
#include "inode.h"

#define MAX_FILE_NAME_LEN 16

typedef enum {
    FT_UNKNOWN,
    FT_REGULAR,
    FT_DIRECTORY
}FileType;

typedef struct dir {
    Inode* inode;
    uint32 dirPos;
    uint8 dirBuf[512];
} Dir;

typedef struct dirEntry {
    char fileName[MAX_FILE_NAME_LEN];
    uint32 ino;
    FileType fileType;
} DirEntry;

void openRootDir(Partition* part);

Dir* dirOpen(Partition* part, uint32 ino);

bool searchDirEntry(Partition* part, Dir* dir, const char* name, DirEntry* de);

void dirClose(Dir* dir);

void createDirEntry(char* fileName, uint32 ino, FileType fileType, DirEntry* de);

bool syncDirEntry(Dir* parentDir, DirEntry* de, void* ioBuf);

void printDirEntry(Dir* parentDir);

bool deleteDirEntry(Partition* part, Dir* dir, uint32 ino, void* ioBuf);

DirEntry* dirRead(Dir* dir);

bool dirIsEmpty(Dir* dir);

int32 dirRemove(Dir* parentDir, Dir* chileDir);
#endif