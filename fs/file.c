#include "file.h"
#include "fs.h"
#include "dir.h"
#include "../lib/kernel/stdio.h"
#include "../lib/string.h"
#include "../lib/debug.h"
#include "../kernel/thread/thread.h"
#include "../kernel/memory.h"
#include "../device/ide.h"

File fileTable[MAX_FILE_OPEN_ALL];
extern Partition* curPart;

int32 getFreeSlotInGlobal() {
    uint32 fdIdx = 3;
    while (fdIdx < MAX_FILE_OPEN_ALL) {
        if (fileTable[fdIdx].fdInode == NULL) {
            break;
        }
        fdIdx++;
    }
    if (fdIdx == MAX_FILE_OPEN_ALL) {
        printk("getFreeSlotInGlobal() exceed max open files\n");
        return -1;
    }
    return fdIdx;
}

int32 pcbFdInstall(int32 globalFdIdx) {
    TaskStruct* cur = getCurrentThread();
    uint32 localFdIdx = 3;
    while (localFdIdx < MAX_FILE_OPEN_PER_PROC) {
        if (cur->fdTable[localFdIdx] == -1) {
            cur->fdTable[localFdIdx] = globalFdIdx;
            break;
        }
        localFdIdx++;
    }
    if (localFdIdx == MAX_FILE_OPEN_PER_PROC) {
        printk("pcbFdInstall() exceed max open files per proc\n");
        return -1;
    }
    return localFdIdx;
}

int32 inodeBitMapAlloc(Partition* part) {
    int32 bitIdx = bitMapScanAndSet(&part->inodeBitMap, 1, 1);
    if (bitIdx == -1) {
        return -1;
    }
    return bitIdx;
}

int32 blockBitMapAlloc(Partition* part) {
    int32 bitIdx = bitMapScanAndSet(&part->blockBitMap, 1, 1);
    if (bitIdx == -1) {
        return -1;
    }
    return part->superBlock->dataLbaStart + bitIdx;
}

void bitMapSync(Partition* part, uint32 bitIdx, BitMapType btype) {
    uint32 secOffset = bitIdx / BITS_PER_SECTOR;
    uint32 byteOffset = secOffset * BLOCK_BYTE_SIZE;
    uint32 secLba;
    uint8* bitMapOff;
    switch (btype) {
        case INODE_BITMAP:
            secLba = part->superBlock->inodeBitMapLbaStart + secOffset;
            bitMapOff = part->inodeBitMap.startAddr + byteOffset;
            break;
        case BLOCK_BITMAP:
            secLba = part->superBlock->blockBitMapLbaStart + secOffset;
            bitMapOff = part->blockBitMap.startAddr + byteOffset;
            break;
    }
    ideWrite(part->disk, secLba, bitMapOff, 1);
}

int32 fileCreate(Dir* parentDir, char* fileName, OFlags flag) {
    void* ioBuf = sysMalloc(1024);
    if (ioBuf == NULL) {
        printk("fileCreate ioBuf = sysMalloc(1024) failed\n");
        return -1;
    }
    uint8 rollBackStep = 0;
    int32 ino = inodeBitMapAlloc(curPart);
    if (ino == -1) {
        printk("ino = inodeBitMapAlloc(curPart) failed\n");
        return -1;
    }
    Inode* newFileInode = (Inode*) sysMalloc(sizeof(Inode));
    if (newFileInode == NULL) {
        printk("newFileInode = (Inode*) sysMalloc(sizeof(Inode)) failed\n");
        rollBackStep = 1;
        goto rollback;
    }
    inodeInit(ino, newFileInode);
    int fdIdx = getFreeSlotInGlobal();
    if (fdIdx == -1) {
        printk("fdIdx = getFreeSlotInGlobal() file number exceeded\n");
        rollBackStep = 2;
        goto rollback;
    }
    fileTable[fdIdx].fdInode = newFileInode;
    fileTable[fdIdx].fdPos = 0;
    fileTable[fdIdx].fdFlag = flag;
    fileTable[fdIdx].fdInode->writeDeny = false;

    DirEntry newDirEntry;
    memset(&newDirEntry, 0, sizeof(DirEntry));
    createDirEntry(fileName, ino, FT_REGULAR, &newDirEntry);
    if (!syncDirEntry(parentDir, &newDirEntry, ioBuf)) {
        printk("syncDirEntry(parentDir, &newDirEntry, ioBuf) failed\n");
        rollBackStep = 3;
        goto rollback;
    }
    memset(ioBuf, 0, 1024);
    inodeSync(curPart, parentDir->inode, ioBuf);
    memset(ioBuf, 0, 1024);
    inodeSync(curPart, newFileInode, ioBuf);
    bitMapSync(curPart, ino, INODE_BITMAP);
    listPush(&curPart->openInodes, &newFileInode->inodeTag);
    newFileInode->openCnt = 1;
    sysFree(ioBuf);
    return pcbFdInstall(fdIdx);
    rollback:
    switch (rollBackStep) {
        case 3:
            memset(&fileTable[fdIdx], 0, sizeof(File));
        case 2:
            sysFree(newFileInode);
        case 1:
            bitMapSet(&curPart->inodeBitMap, ino, 0);
            break;
    }
    sysFree(ioBuf);
    return -1;
}

int32 fileOpen(uint32 ino, OFlags flag) {
    int32 fdIdx = getFreeSlotInGlobal();

    if (fdIdx == -1) {
        printk("fileOpen:exceed max open files\n");
        return -1;
    }
    fileTable[fdIdx].fdInode = inodeOpen(curPart, ino);
    fileTable[fdIdx].fdPos = 0;
    fileTable[fdIdx].fdFlag = flag;
    bool* writeDeny = &fileTable[fdIdx].fdInode->writeDeny;
    if (flag & O_WRONLY || flag & O_RDWR) {
        IntrStatus intrStatus = getIntrStatus();
        disableIntr();
        if (!(*writeDeny)) {
            *writeDeny = true;
            setIntrStatus(intrStatus);
        } else {
            inodeClose(fileTable[fdIdx].fdInode);
            fileTable[fdIdx].fdInode = NULL;
            setIntrStatus(intrStatus);
            printk("file can't be written now!");
            return -1;
        }
    }
    return pcbFdInstall(fdIdx);
}

int32 fileClose(File* file) {
    if (file == NULL) {
        return -1;
    }
    file->fdInode->writeDeny = false;
    inodeClose(file->fdInode);
    file->fdInode = NULL;
    return 0;
}

int32 fileWrite(File* file, const void* buf, uint32 count) {
    if ((file->fdInode->size + count) > (BLOCK_BYTE_SIZE * 140)) {
        printk("exceed max file size 71680 byte, write file failed\n");
        return -1;
    }
    uint8* ioBuf = sysMalloc(2 * SECTOR_BYTE_SIZE);
    if (ioBuf == NULL) {
        printk("fileWrite: ioBuf = sysMalloc(SECTOR_BYTE_SIZE) failed\n");
        return -1;
    }
    uint32* allBlocks = (uint32*) sysMalloc(560);
    if (allBlocks == NULL) {
        printk("fileWrite: allBlocks = (uint32*) sysMalloc(560) failed\n");
        return -1;
    }

    const uint8* sec = buf;
    int32 blockLba = -1;
    uint32 bytesWritten = 0;
    uint32 sizeLeft = count;
    uint32 blockBitMapIdx = 0;
    uint32 secIdx;
    uint32 secLba;
    uint32 secOffBytes;
    uint32 secLeftBytes;
    uint32 chunkSize;
    int32 indirectBlockTable;
    uint32 blockIdx;

    if (file->fdInode->block[0] == 0) {
        blockLba = blockBitMapAlloc(curPart);
        if (blockLba == -1) {
            printk("fileWrite:blockLba = blockBitMapAlloc(curPart) failed\n");
            return -1;
        }
        file->fdInode->block[0] = blockLba;
        blockBitMapIdx = blockLbaToBitMapIdx(blockLba);
        ASSERT(blockBitMapIdx != 0)
        bitMapSync(curPart, blockBitMapIdx, BLOCK_BITMAP);
    }

    uint32 fileHasUsedBlocks = file->fdInode->size / BLOCK_BYTE_SIZE + 1;
    uint32 fileWillUseBlocks = (file->fdInode->size + count) / BLOCK_BYTE_SIZE + 1;
    ASSERT(fileWillUseBlocks <= 140)

    uint32 addBlocks = fileWillUseBlocks - fileHasUsedBlocks;
    if (addBlocks == 0) {
        if (fileWillUseBlocks <= 12) {
            blockIdx = fileHasUsedBlocks - 1;
            ASSERT(file->fdInode->block[blockIdx] != 0)
            allBlocks[blockIdx] = file->fdInode->block[blockIdx];
        } else {
            ASSERT(file->fdInode->block[12] != 0)
            indirectBlockTable = file->fdInode->block[12];
            ideRead(curPart->disk, indirectBlockTable, allBlocks + 12, 1);
        }
    } else {
        if (fileWillUseBlocks <= 12) {
            blockIdx = fileHasUsedBlocks - 1;
            ASSERT(file->fdInode->block[blockIdx] != 0)
            allBlocks[blockIdx] = file->fdInode->block[blockIdx];
            blockIdx = fileHasUsedBlocks;
            while (blockIdx < fileWillUseBlocks) {
                blockLba = blockBitMapAlloc(curPart);
                if (blockLba == -1) {
                    printk("fileWrite:blockLba = blockBitMapAlloc(curPart) situation 1 failed\n");
                    return -1;
                }
                ASSERT(file->fdInode->block[blockIdx] == 0)
                file->fdInode->block[blockIdx] = blockLba;
                allBlocks[blockIdx] = blockLba;
                blockBitMapIdx = blockLbaToBitMapIdx(blockLba);
                ASSERT(blockBitMapIdx != 0)
                bitMapSync(curPart, blockBitMapIdx, BLOCK_BITMAP);
                blockIdx++;
            }
        } else if (fileHasUsedBlocks <= 12 && fileWillUseBlocks > 12) {
            blockIdx = fileHasUsedBlocks - 1;
            ASSERT(file->fdInode->block[blockIdx] != 0)
            allBlocks[blockIdx] = file->fdInode->block[blockIdx];
            blockLba = blockBitMapAlloc(curPart);
            if (blockLba == -1) {
                printk("fileWrite:blockLba = blockBitMapAlloc(curPart) situation 2 failed\n");
                return -1;
            }
            ASSERT(file->fdInode->block[12] == 0)
            file->fdInode->block[12] = blockLba;
            indirectBlockTable = blockLba;
            blockIdx = fileHasUsedBlocks;
            while (blockIdx < fileWillUseBlocks) {
                blockLba = blockBitMapAlloc(curPart);
                if (blockLba == -1) {
                    printk("fileWrite:blockLba = blockBitMapAlloc(curPart) situation 3 failed\n");
                    return -1;
                }
                if (blockIdx < 12) {
                    ASSERT(file->fdInode->block[blockIdx] == 0)
                    file->fdInode->block[blockIdx] = allBlocks[blockIdx] = blockLba;
                } else {
                    allBlocks[blockIdx] = blockLba;
                }
                blockBitMapIdx = blockLbaToBitMapIdx(blockLba);
                bitMapSync(curPart, blockBitMapIdx, BLOCK_BITMAP);
                blockIdx++;
            }
            ideWrite(curPart->disk, indirectBlockTable, allBlocks + 12, 1);
        } else if (fileHasUsedBlocks > 12) {
            ASSERT(file->fdInode->block[12] != 0)
            indirectBlockTable = file->fdInode->block[12];
            ideRead(curPart->disk, indirectBlockTable, allBlocks + 12, 1);
            blockIdx = fileHasUsedBlocks;
            while (blockIdx < fileWillUseBlocks) {
                blockLba = blockBitMapAlloc(curPart);
                if (blockLba == -1) {
                    printk("fileWrite:blockLba = blockBitMapAlloc(curPart) situation 4 failed\n");
                    return -1;
                }
                allBlocks[blockIdx] = blockLba;
                blockBitMapIdx = blockLbaToBitMapIdx(blockLba);
                bitMapSync(curPart, blockBitMapIdx, BLOCK_BITMAP);
                blockIdx++;
            }
            ideWrite(curPart->disk, indirectBlockTable, allBlocks + 12, 1);
        }
    }
    bool firstWriteBlock = true;
    file->fdPos = file->fdInode->size - 1;
    while (bytesWritten < count) {
        memset(ioBuf, 0, BLOCK_BYTE_SIZE);
        secIdx = file->fdInode->size / BLOCK_BYTE_SIZE;
        secLba = allBlocks[secIdx];
        secOffBytes = ile->fdInode->size % BLOCK_BYTE_SIZE;
        secLeftBytes = BLOCK_BYTE_SIZE - secOffBytes;
        chunkSize = sizeLeft < secLeftBytes ? sizeLeft : secLeftBytes;
        if (firstWriteBlock) {
            ideRead(curPart->disk, secLba, ioBuf, 1);
            firstWriteBlock = false;
        }
        memcpy(ioBuf + secOffBytes, src, chunkSize);
        ideWrite(curPart->disk, secLba, ioBuf, 1);
        printk("file write at lba %x", secLba);
        src += chunkSize;
        file->fdInode->size += chunkSize;
        file->fdPos += chunkSize;
        bytesWritten += chunkSize;
        sizeLeft -= chunkSize;
    }
    inodeSync(curPart, file->fdInode, ioBuf);
    sysFree(ioBuf);
    sysFree(allBlocks);
    return bytesWritten;
}

int32 fileRead(File* file, const void* buf, uint32 count) {
    uint8* dst;
    uint32 size = count;
    uint32 sizeLeft = size;
    if ((file->fdPos + count) > file->fdInode->size) {
        size = file->fdInode->size - file->fdPos;
        sizeLeft = size;
        if (size == 0) {
            return -1;
        }
    }
    uint8* ioBuf = sysMalloc(BLOCK_BYTE_SIZE);
    if (ioBuf == NULL) {
        printk("fileRead: ioBuf = sysMalloc(BLOCK_BYTE_SIZE) failed");
        return -1;
    }
    uint32* allBlocks = (uint32*) sysMalloc(560);
    if (allBlocks == NULL) {
        printk("fileRead: allBlocks = (uint32*) sysMalloc(560) failed");
        return -1;
    }
    uint32 blockReadStartIdx = file->fdPos / BLOCK_BYTE_SIZE;
    uint32 blockReadEndIdx = (file->fdPos + size) / BLOCK_BYTE_SIZE;
    uint32 readBlocks = blockReadStartIdx - blockReadStartIdx;
    ASSERT(blockReadStartIdx<=139 && blockReadEndIdx<139)



    const uint8* sec = buf;
    int32 blockLba = -1;
    uint32 bytesWritten = 0;
    uint32 sizeLeft = count;
    uint32 blockBitMapIdx = 0;
    uint32 secIdx;
    uint32 secLba;
    uint32 secOffBytes;
    uint32 secLeftBytes;
    uint32 chunkSize;
    int32 indirectBlockTable;
    uint32 blockIdx;

    if (file->fdInode->block[0] == 0) {
        blockLba = blockBitMapAlloc(curPart);
        if (blockLba == -1) {
            printk("fileWrite:blockLba = blockBitMapAlloc(curPart) failed\n");
            return -1;
        }
        file->fdInode->block[0] = blockLba;
        blockBitMapIdx = blockLbaToBitMapIdx(blockLba);
        ASSERT(blockBitMapIdx != 0)
        bitMapSync(curPart, blockBitMapIdx, BLOCK_BITMAP);
    }

    uint32 fileHasUsedBlocks = file->fdInode->size / BLOCK_BYTE_SIZE + 1;
    uint32 fileWillUseBlocks = (file->fdInode->size + count) / BLOCK_BYTE_SIZE + 1;
    ASSERT(fileWillUseBlocks <= 140)

    uint32 addBlocks = fileWillUseBlocks - fileHasUsedBlocks;
    if (addBlocks == 0) {
        if (fileWillUseBlocks <= 12) {
            blockIdx = fileHasUsedBlocks - 1;
            ASSERT(file->fdInode->block[blockIdx] != 0)
            allBlocks[blockIdx] = file->fdInode->block[blockIdx];
        } else {
            ASSERT(file->fdInode->block[12] != 0)
            indirectBlockTable = file->fdInode->block[12];
            ideRead(curPart->disk, indirectBlockTable, allBlocks + 12, 1);
        }
    } else {
        if (fileWillUseBlocks <= 12) {
            blockIdx = fileHasUsedBlocks - 1;
            ASSERT(file->fdInode->block[blockIdx] != 0)
            allBlocks[blockIdx] = file->fdInode->block[blockIdx];
            blockIdx = fileHasUsedBlocks;
            while (blockIdx < fileWillUseBlocks) {
                blockLba = blockBitMapAlloc(curPart);
                if (blockLba == -1) {
                    printk("fileWrite:blockLba = blockBitMapAlloc(curPart) situation 1 failed\n");
                    return -1;
                }
                ASSERT(file->fdInode->block[blockIdx] == 0)
                file->fdInode->block[blockIdx] = blockLba;
                allBlocks[blockIdx] = blockLba;
                blockBitMapIdx = blockLbaToBitMapIdx(blockLba);
                ASSERT(blockBitMapIdx != 0)
                bitMapSync(curPart, blockBitMapIdx, BLOCK_BITMAP);
                blockIdx++;
            }
        } else if (fileHasUsedBlocks <= 12 && fileWillUseBlocks > 12) {
            blockIdx = fileHasUsedBlocks - 1;
            ASSERT(file->fdInode->block[blockIdx] != 0)
            allBlocks[blockIdx] = file->fdInode->block[blockIdx];
            blockLba = blockBitMapAlloc(curPart);
            if (blockLba == -1) {
                printk("fileWrite:blockLba = blockBitMapAlloc(curPart) situation 2 failed\n");
                return -1;
            }
            ASSERT(file->fdInode->block[12] == 0)
            file->fdInode->block[12] = blockLba;
            indirectBlockTable = blockLba;
            blockIdx = fileHasUsedBlocks;
            while (blockIdx < fileWillUseBlocks) {
                blockLba = blockBitMapAlloc(curPart);
                if (blockLba == -1) {
                    printk("fileWrite:blockLba = blockBitMapAlloc(curPart) situation 3 failed\n");
                    return -1;
                }
                if (blockIdx < 12) {
                    ASSERT(file->fdInode->block[blockIdx] == 0)
                    file->fdInode->block[blockIdx] = allBlocks[blockIdx] = blockLba;
                } else {
                    allBlocks[blockIdx] = blockLba;
                }
                blockBitMapIdx = blockLbaToBitMapIdx(blockLba);
                bitMapSync(curPart, blockBitMapIdx, BLOCK_BITMAP);
                blockIdx++;
            }
            ideWrite(curPart->disk, indirectBlockTable, allBlocks + 12, 1);
        } else if (fileHasUsedBlocks > 12) {
            ASSERT(file->fdInode->block[12] != 0)
            indirectBlockTable = file->fdInode->block[12];
            ideRead(curPart->disk, indirectBlockTable, allBlocks + 12, 1);
            blockIdx = fileHasUsedBlocks;
            while (blockIdx < fileWillUseBlocks) {
                blockLba = blockBitMapAlloc(curPart);
                if (blockLba == -1) {
                    printk("fileWrite:blockLba = blockBitMapAlloc(curPart) situation 4 failed\n");
                    return -1;
                }
                allBlocks[blockIdx] = blockLba;
                blockBitMapIdx = blockLbaToBitMapIdx(blockLba);
                bitMapSync(curPart, blockBitMapIdx, BLOCK_BITMAP);
                blockIdx++;
            }
            ideWrite(curPart->disk, indirectBlockTable, allBlocks + 12, 1);
        }
    }
    bool firstWriteBlock = true;
    file->fdPos = file->fdInode->size - 1;
    while (bytesWritten < count) {
        memset(ioBuf, 0, BLOCK_BYTE_SIZE);
        secIdx = file->fdInode->size / BLOCK_BYTE_SIZE;
        secLba = allBlocks[secIdx];
        secOffBytes = ile->fdInode->size % BLOCK_BYTE_SIZE;
        secLeftBytes = BLOCK_BYTE_SIZE - secOffBytes;
        chunkSize = sizeLeft < secLeftBytes ? sizeLeft : secLeftBytes;
        if (firstWriteBlock) {
            ideRead(curPart->disk, secLba, ioBuf, 1);
            firstWriteBlock = false;
        }
        memcpy(ioBuf + secOffBytes, src, chunkSize);
        ideWrite(curPart->disk, secLba, ioBuf, 1);
        printk("file write at lba %x", secLba);
        src += chunkSize;
        file->fdInode->size += chunkSize;
        file->fdPos += chunkSize;
        bytesWritten += chunkSize;
        sizeLeft -= chunkSize;
    }
    inodeSync(curPart, file->fdInode, ioBuf);
    sysFree(ioBuf);
    sysFree(allBlocks);
    return bytesWritten;
}