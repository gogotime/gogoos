
#include "../device/ide.h"
#include "../kernel/global.h"
#include "fs.h"
#include "dir.h"
#include "super_block.h"
#include "../lib/debug.h"
#include "../lib/kernel/stdio.h"

static void partitionFormat(Disk* hd, Partition* partition) {
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
    sb.magicNum = 0x19590318;
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

}

bool printPartitionFormat(ListElem* elem, int unusedArg) {
    Partition* partition = elemToEntry(Partition, partTag, elem);
    partitionFormat(partition->disk, partition);
    return false;

}

extern List partitionList;

void fsInit() {
//    listTraversal(&partitionList, printPartitionFormat, 0);
}