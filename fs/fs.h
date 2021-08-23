#ifndef FS_FS_H
#define FS_FS_H

#include "../lib/stdint.h"
#include "inode.h"

#define MAX_FILE_PER_PART 4096
#define SECTOR_BYTE_SIZE 512
#define BITS_PER_SECTOR SECTOR_BYTE_SIZE*8
#define BLOCK_BYTE_SIZE SECTOR_BYTE_SIZE

typedef enum {
    FT_UNKNOWN,
    FT_REGULAR,
    FT_DIRECTORY
}FileType;

void fsInit();
#endif