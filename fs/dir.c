#include "file.h"
#include "fs.h"
#include "dir.h"
#include "inode.h"
#include "../lib/kernel/stdio.h"
#include "../kernel/thread/thread.h"
#include "../kernel/memory.h"
#include "../device/ide.h"
#include "../lib/debug.h"
#include "../lib/string.h"

Dir rootDir;
extern Partition* curPart;

void openRootDir(Partition* part) {
    rootDir.inode = inodeOpen(part, part->superBlock->rootInodeNo);
    rootDir.dirPos = 0;
    memset(rootDir.dirBuf, 0, 512);
}

Dir* dirOpen(Partition* part, uint32 ino) {
    Dir* dir = (Dir*) sysMalloc(sizeof(Dir));
    dir->inode = inodeOpen(part, ino);
    dir->dirPos = 0;
    return dir;
}

bool searchDirEntry(Partition* part, Dir* dir, const char* name, DirEntry* de) {
    uint32 blockCnt = 140;
    uint32* allBlocks = (uint32*) sysMalloc(blockCnt * sizeof(uint32));
    ASSERT(blockCnt * sizeof(uint32) == 560)
    if (allBlocks == NULL) {
        printk("searchDirEntry: sysMalloc for allBlocks failed");
        return false;
    }
    uint32 blockIdx = 0;
    while (blockIdx < 12) {
        allBlocks[blockIdx] = dir->inode->block[blockIdx];
        blockIdx++;
    }
    blockIdx = 0;
    if (dir->inode->block[12] != 0) {
        ideRead(part->disk, dir->inode->block[12], allBlocks + 12, 1);
    }
    uint8* buf = (uint8*) sysMalloc(SECTOR_BYTE_SIZE);
    DirEntry* curDe = (DirEntry*) buf;
    uint32 dirEntrySize = part->superBlock->dirEntrySize;
    uint32 dirEntryCnt = SECTOR_BYTE_SIZE / dirEntrySize;
    while (blockIdx < blockCnt) {
        if (allBlocks[blockIdx] == 0) {
            blockIdx++;
            continue;
        }
        ideRead(part->disk, allBlocks[blockIdx], buf, 1);
        uint32 dieEntryIdx = 0;
        while (dieEntryIdx < dirEntrySize) {
            if (!strcmp(curDe->fileName, name)) {
                memcpy(de, curDe, dirEntrySize);
                sysFree(buf);
                sysFree(allBlocks);
                return true;
            }
            dieEntryIdx++;
            curDe++;
        }
        blockIdx++;
        curDe = (DirEntry*) buf;
        memset(buf, 0, SECTOR_BYTE_SIZE);
    }
    sysFree(buf);
    sysFree(allBlocks);
    return false;
}

void dirClose(Dir* dir) {
    if (dir == &rootDir) {
        return;
    }
    inodeClose(dir->inode);
    sysFree(dir);
}

void createDirEntry(char* fileName, uint32 ino, FileType fileType, DirEntry* de) {
    ASSERT(strlen(fileName) <= MAX_FILE_NAME_LEN)
    memcpy(de->fileName, fileName, strlen(fileName));
    de->ino = ino;
    de->fileType = fileType;
}

bool syncDirEntry(Dir* parentDir, DirEntry* de, void* ioBuf) {
    Inode* dirInode = parentDir->inode;
    uint32 dirSize = dirInode->size;
    uint32 dirEntrySize = curPart->superBlock->dirEntrySize;
    ASSERT(dirSize % dirEntrySize == 0)
    uint32 dirEntryCntPerSec = SECTOR_BYTE_SIZE / dirEntrySize;
    int32 blockLba = -1;
    uint8 blockIdx = 0;
    uint32 blockCnt = 140;
    uint32* allBlocks = (uint32*) sysMalloc(blockCnt * sizeof(uint32));
    ASSERT(blockCnt * sizeof(uint32) == 560)
    if (allBlocks == NULL) {
        printk("searchDirEntry: sysMalloc for allBlocks failed");
        return false;
    }
    while (blockIdx < 12) {
        allBlocks[blockIdx] = dirInode->block[blockIdx];
        blockIdx++;
    }
    if (dirInode->block[12] != 0) {
        ideRead(curPart->disk, dirInode->block[12], allBlocks + 12, 1);
    }
    DirEntry* curDe = (DirEntry*) ioBuf;
    int32 blockBitMapIdx = -1;
    blockIdx = 0;
    while (blockIdx < blockCnt) {
        blockBitMapIdx = -1;
        if (allBlocks[blockIdx] == 0) {
            blockLba = blockBitMapAlloc(curPart);
            if (blockLba == -1) {
                printk("syncDirEntry blockBitMapAlloc failed 11\n");
                sysFree(allBlocks);
                return false;
            }
            blockBitMapIdx = blockLbaToBitMapIdx(blockLba);
            ASSERT(blockBitMapIdx != -1)
            bitMapSync(curPart, blockBitMapIdx, BLOCK_BITMAP);
            blockBitMapIdx = -1;
            if (blockIdx < 12) {
                dirInode->block[blockIdx] = blockLba;
                allBlocks[blockIdx] = blockLba;
            } else if (blockIdx == 12) {
                dirInode->block[12] = blockLba;
                blockLba = -1;
                blockLba = blockBitMapAlloc(curPart);
                if (blockLba == -1) {
                    blockBitMapIdx = blockLbaToBitMapIdx(dirInode->block[12]);
                    bitMapSet(&curPart->blockBitMap, blockBitMapIdx, 0);
                    dirInode->block[12] = 0;
                    bitMapSync(curPart, blockBitMapIdx, BLOCK_BITMAP);
                    printk("syncDirEntry blockBitMapAlloc failed 22\n");
                    sysFree(allBlocks);
                    return false;
                }
                blockBitMapIdx = blockLbaToBitMapIdx(blockLba);
                ASSERT(blockBitMapIdx != -1)
                bitMapSync(curPart, blockBitMapIdx, BLOCK_BITMAP);
                allBlocks[12] = blockLba;
                ideWrite(curPart->disk, dirInode->block[12], allBlocks + 12, 1);
            } else {
                allBlocks[blockIdx] = blockLba;
                ideWrite(curPart->disk, dirInode->block[12], allBlocks + 12, 1);
            }
            memset(ioBuf, 0, 512);
            memcpy(ioBuf, de, dirEntrySize);
            ideWrite(curPart->disk, allBlocks[blockIdx], ioBuf, 1);
            dirInode->size += dirEntrySize;
            sysFree(allBlocks);
            return true;
        }
        ideRead(curPart->disk, allBlocks[blockIdx], ioBuf, 1);
        DirEntry* curDe = (DirEntry*) ioBuf;
        uint32 dirEntryIdx = 0;
        while (dirEntryIdx < dirEntryCntPerSec) {
            if ((curDe + dirEntryIdx)->fileType == FT_UNKNOWN) {
                memcpy(curDe + dirEntryIdx, de, dirEntrySize);
                ideWrite(curPart->disk, allBlocks[blockIdx], ioBuf, 1);
                dirInode->size += dirEntrySize;
                sysFree(allBlocks);
                return true;
            }
            dirEntryIdx++;
        }
        blockIdx++;
    }
    printk("directory is full\n");
    sysFree(allBlocks);
    return false;
}

void printDirEntry(Dir* parentDir) {
    Inode* dirInode = parentDir->inode;
    uint32 dirSize = dirInode->size;
    uint32 dirEntrySize = curPart->superBlock->dirEntrySize;
    ASSERT(dirSize % dirEntrySize == 0)
    uint32 dirEntryCntPerSec = SECTOR_BYTE_SIZE / dirEntrySize;
    uint8 blockIdx = 0;
    uint32 blockCnt = 140;
    uint32* allBlocks = (uint32*) sysMalloc(blockCnt * sizeof(uint32));
    ASSERT(blockCnt * sizeof(uint32) == 560)
    if (allBlocks == NULL) {
        printk("printDirEntry: sysMalloc for allBlocks failed");
        return;
    }
    while (blockIdx < 12) {
        allBlocks[blockIdx] = dirInode->block[blockIdx];
        blockIdx++;
    }
    if (dirInode->block[12] != 0) {
        ideRead(curPart->disk, dirInode->block[12], allBlocks + 12, 1);
    }
    uint8* ioBuf = (uint8*) sysMalloc(SECTOR_BYTE_SIZE);

    blockIdx = 0;
    bool flag = true;
    while (blockIdx < blockCnt && flag) {
        ideRead(curPart->disk, allBlocks[blockIdx], ioBuf, 1);
        DirEntry* curDe = (DirEntry*) ioBuf;
        for (uint8 deIdx = 0; deIdx < dirEntryCntPerSec; deIdx++) {
            if ((curDe->fileType == FT_DIRECTORY) || (curDe->fileType == FT_REGULAR)) {
                printk("%s:%d ", curDe->fileName,curDe->ino);
            } else {
                flag = false;
            }
            curDe++;
        }
    }
    printk("\n");
    sysFree(allBlocks);
    sysFree(ioBuf);
}