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
    uint32 blockCnt = 12;
    uint32* allBlocks = (uint32*) sysMalloc(560);
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
        blockCnt = 140;
    }
    uint8* ioBuf = (uint8*) sysMalloc(SECTOR_BYTE_SIZE);

    blockIdx = 0;
    bool flag = true;
    while (blockIdx < blockCnt && flag) {
        if (allBlocks[blockIdx] == 0) {
            continue;
        }
        ideRead(curPart->disk, allBlocks[blockIdx], ioBuf, 1);
        DirEntry* curDe = (DirEntry*) ioBuf;
        for (uint8 deIdx = 0; deIdx < dirEntryCntPerSec; deIdx++) {
            if ((curDe->fileType == FT_DIRECTORY) || (curDe->fileType == FT_REGULAR)) {
                printk("%s:%d ", curDe->fileName, curDe->ino);
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

bool deleteDirEntry(Partition* part, Dir* dir, uint32 ino, void* ioBuf) {
    Inode* dirInode = dir->inode;
    uint32 blockIdx = 0;
    uint32* allBlocks = (uint32*) sysMalloc(560);
    while (blockIdx < 12) {
        allBlocks[blockIdx] = dirInode->block[blockIdx];
        blockIdx++;
    }
    if (dirInode->block[12] != 0) {
        ideRead(part->disk, dirInode->block[12], allBlocks + 12, 1);
    }
    DirEntry* curDe = (DirEntry*) ioBuf;
    DirEntry* deFound = NULL;
    uint32 dirEntrySize = part->superBlock->dirEntrySize;
    uint32 dirEntryCntPerSec = SECTOR_BYTE_SIZE / dirEntrySize;
    uint32 dirEntryIdx;
    uint32 dirEntryCnt;
    bool isDirFirstBlock = false;
    blockIdx = 0;
    while (blockIdx < 140) {
        isDirFirstBlock = false;
        if (allBlocks[blockIdx] == 0) {
            blockIdx++;
            continue;
        }
        dirEntryIdx = dirEntryCnt = 0;
        memset(ioBuf, 0, SECTOR_BYTE_SIZE);
        ideRead(part->disk, allBlocks[blockIdx], ioBuf, 1);

        while (dirEntryIdx < dirEntryCntPerSec) {
            if ((curDe + dirEntryIdx)->fileType != FT_UNKNOWN) {
                if (!strcmp((curDe + dirEntryIdx)->fileName, ".")) {
                    isDirFirstBlock = true;
                } else if (strcmp((curDe + dirEntryIdx)->fileName, ".") &&
                           strcmp((curDe + dirEntryIdx)->fileName, ".")) {
                    dirEntryCnt++;
                    if ((curDe + dirEntryIdx)->ino == ino) {
                        ASSERT(deFound == NULL)
                        deFound = curDe + dirEntryIdx;
                    }
                }
            }
            dirEntryIdx++;
        }
        if (deFound == NULL) {
            blockIdx++;
            continue;
        }
        ASSERT(dirEntryCnt >= 1)
        if (dirEntryCnt == 1 && !isDirFirstBlock) {
            uint32 blockBitMapIdx = blockLbaToBitMapIdx(allBlocks[blockIdx]);
            bitMapSet(&part->blockBitMap, blockBitMapIdx, 0);
            bitMapSync(part, blockBitMapIdx, BLOCK_BITMAP);
            if (blockIdx < 12) {
                dirInode->block[blockIdx] = 0;
            } else {
                uint32 indirectBlocksCnt = 0;
                uint32 indirectBlocksIdx = 12;
                while (indirectBlocksIdx < 140) {
                    if (allBlocks[indirectBlocksIdx] != 0) {
                        indirectBlocksCnt++;
                    }
                    indirectBlocksIdx++;
                }
                ASSERT(indirectBlocksCnt >= 1)
                if (indirectBlocksCnt > 1) {
                    allBlocks[blockIdx] = 0;
                    ideWrite(part->disk, dirInode->block[12], allBlocks + 12, 1);
                } else {
                    uint32 blockBitMapIdx = blockLbaToBitMapIdx(allBlocks[12]);
                    bitMapSet(&part->blockBitMap, blockBitMapIdx, 0);
                    bitMapSync(part, blockBitMapIdx, BLOCK_BITMAP);
                    dirInode->block[12] = 0;
                }
            }

        } else {
            memset(deFound, 0, dirEntrySize);
            ideWrite(part->disk, allBlocks[blockIdx], ioBuf, 1);
        }
        ASSERT(dirInode->size >= dirEntrySize)
        dirInode->size -= dirEntrySize;
        memset(ioBuf, 0, SECTOR_BYTE_SIZE * 2);
        inodeSync(part, dirInode, ioBuf);
        return true;
    }
    return false;
}

DirEntry* dirRead(Dir* dir) {
    DirEntry* de = (DirEntry*) dir->dirBuf;
    Inode* dirInode = dir->inode;
    uint8 blockIdx = 0;
    uint32 blockCnt = 12;
    uint32* allBlocks = (uint32*) sysMalloc(560);
    if (allBlocks == NULL) {
        printk("dirRead: sysMalloc for allBlocks failed");
        return NULL;
    }
    while (blockIdx < 12) {
        allBlocks[blockIdx] = dirInode->block[blockIdx];
        blockIdx++;
    }
    if (dirInode->block[12] != 0) {
        ideRead(curPart->disk, dirInode->block[12], allBlocks + 12, 1);
        blockCnt = 140;
    }
    blockIdx = 0;
    uint32 curDirEntryPos = 0;
    uint32 dirSize = dirInode->size;
    uint32 dirEntrySize = curPart->superBlock->dirEntrySize;
    ASSERT(dirSize % dirEntrySize == 0)
    uint32 dirEntryIdx = 0;
    uint32 dirEntryCntPerSec = SECTOR_BYTE_SIZE / dirEntrySize;
    while (dir->dirPos < dirInode->size && blockIdx < blockCnt) {
        if (allBlocks[blockIdx] == 0) {
            blockIdx++;
            continue;
        }
        memset(de, 0, SECTOR_BYTE_SIZE);
        ideRead(curPart->disk, allBlocks[blockIdx], de, 1);
        dirEntryIdx = 0;
        while (dirEntryIdx < dirEntryCntPerSec) {
            if ((de + dirEntryIdx)->fileType != FT_UNKNOWN) {
                if (curDirEntryPos < dir->dirPos) {
                    curDirEntryPos += dirEntrySize;
                    dirEntryIdx++;
                    continue;
                }
                ASSERT(curDirEntryPos == dir->dirPos)
                dir->dirPos += dirEntrySize;
                return de + dirEntryIdx;
            }
            dirEntryIdx++;
        }
        blockIdx++;
    }
    return NULL;
}

bool dirIsEmpty(Dir* dir) {
    Inode* dirInode = dir->inode;
    return dirInode->size == curPart->superBlock->dirEntrySize * 2;
}

int32 dirRemove(Dir* parentDir, Dir* chileDir) {
    Inode* childDirInode = chileDir->inode;
    int32 blockIdx = 1;
    while (blockIdx < 13) {
        ASSERT(childDirInode->block[blockIdx]==0)
        blockIdx++;
    }
    void* ioBuf = sysMalloc(SECTOR_BYTE_SIZE * 2);
    if (ioBuf == NULL) {
        printk("dirRemove: ioBuf = sysMalloc(SECTOR_BYTE_SIZE * 2) failed\n");
        return -1;
    }
    deleteDirEntry(curPart, parentDir, childDirInode->ino, ioBuf);
    inodeRelease(curPart, childDirInode->ino);
    sysFree(ioBuf);
    return 0;
}