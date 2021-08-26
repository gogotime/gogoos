
#include "../device/ide.h"
#include "../kernel/global.h"
#include "../kernel/memory.h"
#include "fs.h"
#include "dir.h"
#include "super_block.h"
#include "../lib/debug.h"
#include "../lib/kernel/stdio.h"
#include "../lib/string.h"

#define SUPER_BLOCK_MAGIC_NUM 0x20212021

static void partitionFormat(Partition* partition) {
    uint32 bootSecCnt = 1;
    uint32 superBlockSecCnt = 1;
    uint32 inodeBitMapSecCnt = DIV_ROUND_UP(MAX_FILE_PER_PART, SECTOR_BYTE_SIZE * 8);
    uint32 inodeTableSecCnt = DIV_ROUND_UP(sizeof(Inode) * MAX_FILE_PER_PART, SECTOR_BYTE_SIZE);
    uint32 usedSecCnt = bootSecCnt + superBlockSecCnt + inodeBitMapSecCnt + inodeTableSecCnt;
    uint32 freeSecCnt = partition->secCnt - usedSecCnt;

    uint32 blockBitMapSecCnt = DIV_ROUND_UP(freeSecCnt, BITS_PER_SECTOR);
    uint32 blockBitMapBitCnt = freeSecCnt - blockBitMapSecCnt;
    blockBitMapSecCnt = DIV_ROUND_UP(blockBitMapBitCnt, BITS_PER_SECTOR);

    SuperBlock sb;
    ASSERT(sizeof(SuperBlock) == SECTOR_BYTE_SIZE)
    sb.magicNum = SUPER_BLOCK_MAGIC_NUM;
    sb.secCnt = partition->secCnt;
    sb.inodeCnt = MAX_FILE_PER_PART;
    sb.partLbaStart = partition->lbaStart;

    sb.blockBitMapLbaStart = sb.partLbaStart + 2;
    sb.blockBitMapSecCnt = blockBitMapSecCnt;

    sb.inodeBitMapLbaStart = sb.blockBitMapLbaStart + sb.blockBitMapSecCnt;
    sb.inodeBitMapSecCnt = inodeBitMapSecCnt;

    sb.inodeTableLbaStart = sb.inodeBitMapLbaStart + sb.inodeBitMapSecCnt;
    sb.inodeTableSecCnt = inodeTableSecCnt;

    sb.dataLbaStart = sb.inodeTableLbaStart + inodeTableSecCnt;
    sb.rootInodeNo = 0;
    sb.dirEntrySize = sizeof(Dir);

    printk("    %s info:\n", partition->name);
    printk("        secCnt:%d\n", sb.secCnt);
//    printk("        inodeCnt:%d\n", sb.inodeCnt);
    printk("        partLbaStart:%x\n", sb.partLbaStart);
    printk("        blockBitMapLbaStart:%x\n", sb.blockBitMapLbaStart);
    printk("        blockBitMapSecCnt:%d\n", sb.blockBitMapSecCnt);
//    printk("        inodeBitMapLbaStart:%x\n", sb.inodeBitMapLbaStart);
//    printk("        inodeBitMapSecCnt:%d\n", sb.inodeBitMapSecCnt);
//    printk("        inodeTableLbaStart:%x\n", sb.inodeTableLbaStart);
//    printk("        inodeTableSecCnt:%d\n", sb.inodeTableSecCnt);
    printk("        dataLbaStart:%x\n", sb.dataLbaStart);

    Disk* hd = partition->disk;
    ideWrite(hd, partition->lbaStart + 1, &sb, 1);
    uint32 bufSize = (sb.blockBitMapSecCnt > sb.inodeBitMapSecCnt ? sb.blockBitMapSecCnt : sb.inodeBitMapSecCnt);
    bufSize = (bufSize >= sb.inodeTableSecCnt ? bufSize : sb.inodeTableSecCnt) * SECTOR_BYTE_SIZE;
    uint8* buf = (uint8*) sysMalloc(bufSize);
    printk("buf size:%d", bufSize);
    buf[0] |= 0x01;
    uint32 blockBitMapLastByte = blockBitMapBitCnt / 8;
    uint32 blockBitMapLastBit = blockBitMapBitCnt % 8;
    uint32 lastSize = SECTOR_BYTE_SIZE - (blockBitMapLastByte % SECTOR_BYTE_SIZE);

    memset(&buf[blockBitMapLastByte], 0xff, lastSize);

    uint8 bitIdx = 0;
    while (bitIdx <= blockBitMapLastBit) {
        buf[blockBitMapLastByte] &= ~(1 << bitIdx++);
    }
    ideWrite(hd, sb.blockBitMapLbaStart, buf, sb.blockBitMapSecCnt);

    memset(buf, 0, bufSize);
    buf[0] |= 0x01;
    ideWrite(hd, sb.inodeBitMapLbaStart, buf, sb.inodeBitMapSecCnt);

    memset(buf, 0, bufSize);
    Inode* i = (Inode*) buf;
    i->size = sb.dirEntrySize * 2; //. and ..
    i->ino = 0;
    i->blockIdx[0] = sb.dataLbaStart;
    ideWrite(hd, sb.inodeTableLbaStart, buf, sb.inodeTableSecCnt);

    memset(buf, 0, bufSize);
    DirEntry* dirEntry = (DirEntry*) buf;
    memcpy(dirEntry, ".", 1);
    dirEntry->ino = 0;
    dirEntry->fileType = FT_DIRECTORY;

    dirEntry++;
    memcpy(dirEntry, "..", 1);
    dirEntry->ino = 0;
    dirEntry->fileType = FT_DIRECTORY;

    ideWrite(hd, sb.dataLbaStart, buf, 1);

    printk("    %s format done\n", partition->name);
    sysFree(buf);
}

Partition* curPart;

static bool mountPartition(ListElem* elem, int arg) {
    char* partName = (char*) arg;
    Partition* partition = elemToEntry(Partition, partTag, elem);
    if (strcmp(partition->name, partName)) {
        return false;
    }
    curPart = partition;
    Disk* hd = partition->disk;
    SuperBlock* buf = (SuperBlock*) sysMalloc(SECTOR_BYTE_SIZE);
    curPart->superBlock = (SuperBlock*) sysMalloc(sizeof(SuperBlock));
    if (curPart->superBlock == NULL) {
        PANIC("malloc curPart->superBlock failed")
    }
    memset(buf, 0, SECTOR_BYTE_SIZE);
    ideRead(hd, curPart->lbaStart + 1, buf, 1);
    memcpy(curPart->superBlock, buf, sizeof(SuperBlock));
    curPart->blockBitMap.startAddr = (uint8*) sysMalloc(buf->blockBitMapSecCnt * SECTOR_BYTE_SIZE);
    if (curPart->blockBitMap.startAddr == NULL) {
        PANIC("malloc curPart->blockBitMap.startAddr failed")
    }
    curPart->blockBitMap.length = buf->blockBitMapSecCnt * SECTOR_BYTE_SIZE;
    ideRead(hd, buf->blockBitMapLbaStart, curPart->blockBitMap.startAddr, buf->blockBitMapSecCnt);

    curPart->inodeBitMap.startAddr = (uint8*) sysMalloc(buf->inodeBitMapSecCnt * SECTOR_BYTE_SIZE);
    if (curPart->inodeBitMap.startAddr == NULL) {
        PANIC("malloc curPart->inodeBitMap.startAddr failed")
    }
    curPart->inodeBitMap.length = buf->inodeBitMapSecCnt * SECTOR_BYTE_SIZE;
    ideRead(hd, buf->inodeBitMapLbaStart, curPart->inodeBitMap.startAddr, buf->inodeBitMapSecCnt);

    listInit(&curPart->openInodes);
    printk("mount %s done\n", partition->name);

    return true;
}

extern List partitionList;

void fsInit() {
    ASSERT(sizeof(SuperBlock) == SECTOR_BYTE_SIZE)
    SuperBlock* sb = (SuperBlock*) sysMalloc(SECTOR_BYTE_SIZE);
    ListElem* elem = partitionList.head.next;
    while (elem != &partitionList.tail) {
        Partition* partition = elemToEntry(Partition, partTag, elem);
        ideRead(partition->disk, partition->lbaStart + 1, sb, 1);
        if (sb->magicNum == SUPER_BLOCK_MAGIC_NUM) {
            printk("%s has fileSystem\n", partition->name);
        } else {
            printk("format %s\n", partition->name);
            partitionFormat(partition);
        }
        elem = elem->next;
    }
    char default_part[8] = "sdb1";
    listTraversal(&partitionList, mountPartition, (int) default_part);
}