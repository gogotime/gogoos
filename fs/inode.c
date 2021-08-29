
#include "../lib/stdint.h"
#include "../lib/debug.h"
#include "../lib/string.h"
#include "../lib/kernel/stdio.h"
#include "../device/ide.h"
#include "../kernel/memory.h"
#include "fs.h"

typedef struct {
    bool crossSec;
    uint32 secLba;
    uint32 byteOffset;
} InodePosition;

extern Partition* curPart;

uint32 blockLbaToBitMapIdx(int32 blockLba) {
    return blockLba - curPart->superBlock->dataLbaStart;
}

static void inodeLocate(Partition* part, uint32 ino, InodePosition* ip) {
    ASSERT(ino < 4096)
    uint32 inodeTableLba = part->superBlock->inodeTableLbaStart;
    uint32 inodeSize = sizeof(Inode);
    uint32 byteOffset = ino * inodeSize;
    uint32 secOffset = byteOffset / SECTOR_BYTE_SIZE;
    uint32 byteOffsetInSec = byteOffset % SECTOR_BYTE_SIZE;
    uint32 leftInSec = SECTOR_BYTE_SIZE - byteOffsetInSec;
    if (leftInSec < inodeSize) {
        ip->crossSec = true;
    } else {
        ip->crossSec = false;
    }
    ip->secLba = inodeTableLba + secOffset;
    ip->byteOffset = byteOffsetInSec;
}

void inodeSync(Partition* part, Inode* inode, void* ioBuf) {
    uint32 ino = inode->ino;
    InodePosition ip;
    inodeLocate(part, ino, &ip);
    ASSERT(ip.secLba <= (part->lbaStart + part->secCnt))
    Inode pureInode;
    memcpy(&pureInode, inode, sizeof(Inode));
    pureInode.openCnt = 0;
    pureInode.writeDeny = false;
    pureInode.inodeTag.prev = NULL;
    pureInode.inodeTag.next = NULL;
    char* inodeBuf = (char*) ioBuf;
    if (ip.crossSec) {
        ideRead(part->disk, ip.secLba, inodeBuf, 2);
        memcpy(inodeBuf + ip.byteOffset, &pureInode, sizeof(Inode));
        ideWrite(part->disk, ip.secLba, inodeBuf, 2);
    } else {
        ideRead(part->disk, ip.secLba, inodeBuf, 1);
        memcpy(inodeBuf + ip.byteOffset, &pureInode, sizeof(Inode));
        ideWrite(part->disk, ip.secLba, inodeBuf, 1);
    }
}

Inode* inodeOpen(Partition* part, uint32 ino) {
    ListElem* elem = part->openInodes.head.next;
    Inode* inodeFound;
    while (elem != &part->openInodes.tail) {
        inodeFound = elemToEntry(Inode, inodeTag, elem);
        if (inodeFound->ino == ino) {
            inodeFound->openCnt++;
            return inodeFound;
        }
        elem = elem->next;
    }
    InodePosition ip;
    inodeLocate(part, ino, &ip);
    TaskStruct* cur = getCurrentThread();
    uint32* curPageDirBak = cur->pageDir;
    cur->pageDir = NULL;
    inodeFound = (Inode*) sysMalloc(sizeof(Inode));
    cur->pageDir = curPageDirBak;
    char* inodeBuf;
    if (ip.crossSec) {
        inodeBuf = (char*) sysMalloc(2 * SECTOR_BYTE_SIZE);
        ideRead(part->disk, ip.secLba, inodeBuf, 2);
    } else {
        inodeBuf = (char*) sysMalloc(SECTOR_BYTE_SIZE);
        ideRead(part->disk, ip.secLba, inodeBuf, 1);
    }
    memcpy(inodeFound, inodeBuf + ip.byteOffset, sizeof(Inode));
    listPush(&part->openInodes, &inodeFound->inodeTag);
    inodeFound->openCnt = 1;
    sysFree(inodeBuf);
    return inodeFound;
}

void inodeClose(Inode* inode) {
    IntrStatus intrStatus = getIntrStatus();
    disableIntr();

    inode->openCnt--;
    if (inode->openCnt == 0) {
        listRemove(&inode->inodeTag);
        TaskStruct* cur = getCurrentThread();
        uint32* curPageDirBak = cur->pageDir;
        cur->pageDir = NULL;
        sysFree(inode);
        cur->pageDir = curPageDirBak;
    }
    setIntrStatus(intrStatus);
}

void inodeInit(uint32 ino, Inode* newInode) {
    newInode->ino = ino;
    newInode->size = 0;
    newInode->openCnt = 0;
    newInode->writeDeny = false;
    uint8 blkIdx = 0;
    while (blkIdx < 13) {
        newInode->block[blkIdx] = 0;
        blkIdx++;
    }
}

void inodeDelete(Partition* part, uint32 ino, void* ioBuf) {

}

void inodeRelease(Partition* part, uint32 ino) {
    Inode* inodeToDel = inodeOpen(part, ino);
    ASSERT(inodeToDel->ino == ino)
    uint32 blockIdx = 0;
    uint32 blockCnt = 12;
    uint32 blockBitMapIdx = 0;
    uint32* allBlocks = (uint32*) sysMalloc(560);
    while (blockIdx < 12) {
        allBlocks[blockIdx] = inodeToDel->block[blockIdx];
        blockIdx++;
    }
    if (inodeToDel->block[12] != 0) {
        ideRead(part->disk, inodeToDel->block[12], allBlocks + 12, 1);
        blockIdx = 140;
        blockBitMapIdx = blockLbaToBitMapIdx(inodeToDel->block[12]);
        ASSERT(blockBitMapIdx>0)
        bitMapSet(&part->blockBitMap, blockBitMapIdx, 0);
        bitMapSync(part, blockBitMapIdx, BLOCK_BITMAP);
    }

    blockIdx = 0;
    while (blockIdx < blockCnt) {
        if (allBlocks[blockIdx] != 0) {
            blockBitMapIdx = blockLbaToBitMapIdx(allBlocks[blockIdx]);
            ASSERT(blockBitMapIdx>0)
            bitMapSet(&part->blockBitMap, blockBitMapIdx, 0);
            bitMapSync(part, blockBitMapIdx, BLOCK_BITMAP);
        }
        blockIdx++;
    }
    bitMapSet(&part->inodeBitMap, ino, 0);
    bitMapSync(part, ino, INODE_BITMAP);

    sysFree(allBlocks);
    inodeClose(inodeToDel);

}

