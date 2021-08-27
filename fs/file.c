#include "file.h"
#include "fs.h"
#include "../lib/kernel/stdio.h"
#include "../kernel/thread/thread.h"
#include "../device/ide.h"

File fileTable[MAX_FILE_OPEN_ALL];

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
    int32 bitIdx = bitMapScanAndSet(part->inodeBitMap, 1, 1);
    if (bitIdx == -1) {
        return -1
    }
    return bitIdx;
}

int32 blockBitMapAlloc(Partition* part) {
    int32 bitIdx = bitMapScanAndSet(part->blockBitMap, 1, 1);
    if (bitIdx == -1) {
        return -1
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

