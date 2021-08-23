#ifndef FS_DIR_H
#define FS_DIR_H

#include "../lib/stdint.h"
#include "inode.h"
#include "fs.h"

#define MAX_FILE_NAME_LEN 16

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


#endif