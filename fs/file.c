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
        printk("exceed max open files\n");
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
        printk("exceed max open files per proc\n");
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

int32 fileCreate(Dir* parentDir, char* fileName, uint8 flag) {
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
    createDirEntry(fileName, ino, FT_REGULAR, newDirEntry);
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

