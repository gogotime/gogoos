#include "ide.h"
#include "timer.h"
#include "console.h"
#include "../lib/kernel/stdio.h"
#include "../lib/kernel/io.h"
#include "../lib/debug.h"
#include "../lib/string.h"
#include "../kernel/global.h"
#include "../kernel/interrupt.h"
#include "../kernel/memory.h"

#define regData(channel)     (channel->portBase + 0 )
#define regError(channel)     (channel->portBase + 1 )
#define regSectCnt(channel)     (channel->portBase + 2 )
#define regLbaL(channel)     (channel->portBase + 3 )
#define regLbaM(channel)     (channel->portBase + 4 )
#define regLbaH(channel)     (channel->portBase + 5 )
#define regDev(channel)     (channel->portBase + 6 )
#define regStatus(channel)     (channel->portBase + 7 )
#define regCmd(channel)     (regStatus(channel))
#define regAltStatus(channel)     (channel->portBase + 0x206 )
#define regCtl(channel)     (regAltStatus(channel))

#define BIT_STAT_BSY 0x80
#define BIT_STAT_DRDY 0x40
#define BIT_STAT_DRQ 0x8

#define BIT_DEV_MBS 0xa0
#define BIT_DEV_LBA 0x40
#define BIT_DEV_SLAVE 0x10

#define CMD_IDENTIFY 0xec
#define CMD_READ_SECTOR 0x20
#define CMD_WRITE_SECTOR 0x30

#define MAX_LBA ((80*1024*1024/512)-1)

uint8 channelCnt;
uint8 hdCnt;
IDEChannel ideChannel[2];

uint32 extLbaBase = 0;
uint8 pno = 0;
uint8 lno = 0;
List partitionList;

typedef struct partitionTableEntry PartitionTableEntry;
typedef struct bootSector BootSector;

struct partitionTableEntry {
    uint8 bootable;
    uint8 startHead;
    uint8 startSec;
    uint8 startChs;
    uint8 fsType;
    uint8 endHead;
    uint8 endSec;
    uint8 endChs;
    uint32 startLba;
    uint32 secCnt;
}  __attribute__ ((packed));

struct bootSector {
    uint8 other[446];
    PartitionTableEntry partitionTable[4];
    uint16 signature;
}  __attribute__ ((packed));


static void selectDisk(Disk* hd) {
    uint8 regDevice = BIT_DEV_MBS | BIT_DEV_LBA;
    if (hd->devNo == 1) {
        regDevice |= BIT_DEV_SLAVE;
    }
    outb(regDev(hd->channel), regDevice);
}

static void selectSector(Disk* hd, uint32 lba, uint8 secCnt) {
    ASSERT(lba < MAX_LBA)
    IDEChannel* channel = hd->channel;
    outb(regSectCnt(channel), secCnt);
    outb(regLbaL(channel), lba);
    outb(regLbaM(channel), lba >> 8);
    outb(regLbaH(channel), lba >> 16);
    outb(regDev(channel), lba >> 24 | BIT_DEV_MBS | BIT_DEV_LBA | (hd->devNo == 1 ? BIT_DEV_SLAVE : 0));
}

static void cmdOut(IDEChannel* channel, uint8 cmd) {
    channel->expectingIntr = true;
    outb(regCmd(channel), cmd);
}

static void readSector(Disk* hd, void* buf, uint8 secCnt) {
    uint32 sizeInByte;
    if (secCnt == 0) {
        sizeInByte = 256 * 512;
    } else {
        sizeInByte = secCnt * 512;
    }
    insw(regData(hd->channel), buf, sizeInByte / 2);
}

static void writeSector(Disk* hd, void* buf, uint8 secCnt) {
    uint32 sizeInByte;
    if (secCnt == 0) {
        sizeInByte = 256 * 512;
    } else {
        sizeInByte = secCnt * 512;
    }
    outsw(regData(hd->channel), buf, sizeInByte / 2);
}

static bool busyWait(Disk* hd) {
    IDEChannel* channel = hd->channel;
    uint16 timeLimit = 30 * 1000;
    while (timeLimit > 0) {
        if (!(inb(regStatus(channel)) & BIT_STAT_BSY)) {
            return inb(regStatus(channel)) & BIT_STAT_DRQ;
        } else {
            sleepMs(10);
        }
        timeLimit -= 10;
    }
    return false;
}

void ideRead(Disk* hd, uint32 lba, void* buf, uint32 secCnt) {
    ASSERT(lba < MAX_LBA)
    ASSERT(secCnt > 0)
    ASSERT(getIntrStatus()==INTR_ON)
    lockLock(&hd->channel->lock);
    selectDisk(hd);

    uint32 secsPerLoop;
    uint32 secsDone = 0;
    while (secsDone < secCnt) {
        if (secsDone + 256 <= secCnt) {
            secsPerLoop = 256;
        } else {
            secsPerLoop = secCnt - secsDone;
        }
        selectSector(hd, lba + secsDone, secsPerLoop);
        cmdOut(hd->channel, CMD_READ_SECTOR);
        semaDown(&hd->channel->diskDone);
        if (!busyWait(hd)) {
            char error[64];
            sprintk(error, "%s read sector %d failed!!!\n", hd->name, lba);
            PANIC(error)
        }
        readSector(hd, (void*) ((uint32) buf + secsDone * 512), secsPerLoop);
        secsDone += secsPerLoop;
    }

    lockUnlock(&hd->channel->lock);
}

void ideWrite(Disk* hd, uint32 lba, void* buf, uint32 secCnt) {
    ASSERT(lba < MAX_LBA)
    ASSERT(secCnt > 0)
    lockLock(&hd->channel->lock);
    selectDisk(hd);

    uint32 secsPerLoop;
    uint32 secsDone = 0;
    while (secsDone < secCnt) {
        if (secsDone + 256 <= secCnt) {
            secsPerLoop = 256;
        } else {
            secsPerLoop = secCnt - secsDone;
        }
        selectSector(hd, lba + secsDone, secsPerLoop);
        cmdOut(hd->channel, CMD_WRITE_SECTOR);
        if (!busyWait(hd)) {
            char error[64];
            sprintk(error, "%s write sector %d failed!!!\n", hd->name, lba);
            PANIC(error)
        }
        writeSector(hd, (void*) ((uint32) buf + secsDone * 512), secsPerLoop);
        semaDown(&hd->channel->diskDone);
        secsDone += secsPerLoop;
    }
    lockUnlock(&hd->channel->lock);
}

void intrHdHandler(uint8 intrNo) {
    ASSERT(intrNo == 0x2e || intrNo == 0x2f)
    IDEChannel* channel = &ideChannel[intrNo - 0x2e];
    ASSERT(channel->intrNo == intrNo)
    if (channel->expectingIntr) {
        channel->expectingIntr = false;
        semaUp(&channel->diskDone);
        inb(regStatus(channel));
    }
}

static void swapPairsBytes(const char* src, char* dst, uint32 len) {
    uint32 idx;
    for (idx = 0; idx < len; idx += 2) {
        dst[idx + 1] = *src++;
        dst[idx] = *src++;
    }
    dst[idx] = '\0';
}

static void identifyDisk(Disk* hd) {
    char info[512];
    selectDisk(hd);
    cmdOut(hd->channel, CMD_IDENTIFY);
    semaDown(&hd->channel->diskDone);
    if (!busyWait(hd)) {
        char error[64];
        sprintk(error, "%s identify failed!!!\n", hd->name);
        PANIC(error)
    }
    readSector(hd, info, 1);
    char buf[64];
    uint8 snStart = 20;
    uint8 snLen = 20;
    uint8 mdStart = 27 * 2;
    uint8 mdLen = 40;
    swapPairsBytes(&info[snStart], buf, snLen);
    printk("    disk %s info\n        SN:%s\n", hd->name, buf);
    memset(buf, 0, sizeof(buf));
    swapPairsBytes(&info[mdStart], buf, mdLen);
    printk("        MODULE:%s\n", buf);
    uint32 sectors = *(uint32*) &info[60 * 2];
    printk("        SECTORS:%d\n", sectors);
    printk("        CAPACITY:%dMB\n", sectors * 512 / 1024 / 1024);
}

static void partitionScan(Disk* hd, uint32 extLba) {
    BootSector* bs = sysMalloc(sizeof(BootSector));
    ideRead(hd, extLba, bs, 1);
    uint8 partIdx = 0;
    PartitionTableEntry* p = bs->partitionTable;
    while (partIdx++ < 4) {
        if (p->fsType == 0x5) {
            if (extLbaBase != 0) {
                partitionScan(hd, p->startLba + extLbaBase);
            } else {
                extLbaBase = p->startLba;
                partitionScan(hd, p->startLba);
            }
        } else if (p->fsType != 0) {
            if (extLba == 0) {
                hd->primParts[pno].lbaStart = p->startLba;
                hd->primParts[pno].secCnt = p->secCnt;
                hd->primParts[pno].disk = hd;
                listAppend(&partitionList, &hd->primParts[pno].partTag);
                sprintk(hd->primParts[pno].name, "%s%d", hd->name, pno + 1);
                pno++;
                ASSERT(pno < 4)
            } else {
                hd->logicParts[lno].lbaStart = extLba + p->startLba;
                hd->logicParts[lno].secCnt = p->secCnt;
                hd->logicParts[lno].disk = hd;
                listAppend(&partitionList, &hd->logicParts[lno].partTag);
                sprintk(hd->logicParts[lno].name, "%s%d", hd->name, lno + 5);
                lno++;
                if (lno >= 8) {
                    return;
                }
            }
        }
        p++;
    }
    sysFree(bs);
}

bool printPartitionInfo(ListElem* elem, int unusedArg) {
    Partition* partition = elemToEntry(Partition, partTag, elem);
    printk("        %s startLba:%d , secCnt:%d\n", partition->name, partition->lbaStart, partition->secCnt);
    return false;
}

void ideInit() {
    printk("ideInit start\n");
    ASSERT(sizeof(PartitionTableEntry) == 16)
    ASSERT(sizeof(BootSector) == 512)

    hdCnt = *((uint8*) 0x475);
    ASSERT(hdCnt > 0)
    channelCnt = DIV_ROUND_UP(hdCnt, 2);
    IDEChannel* channel;
    uint8 channelNo = 0;
    uint8 devNo = 0;
    listInit(&partitionList);
    while (channelNo < channelCnt) {
        channel = &ideChannel[channelNo];
        sprintk(channel->name, "ide%d", channelNo);
        switch (channelNo) {
            case 0:
                channel->portBase = 0x1f0;
                channel->intrNo = 0x20 + 14;
                break;
            case 1:
                channel->portBase = 0x170;
                channel->intrNo = 0x20 + 15;
                break;
        }
        channel->expectingIntr = false;
        lockInit(&channel->lock);
        semaInit(&channel->diskDone, 0);
        registerIntrHandler(channel->intrNo, intrHdHandler);
        while (devNo < hdCnt) {
            Disk* hd = &channel->devices[devNo % 2];
            hd->channel = channel;
            hd->devNo = devNo;
            sprintk(hd->name, "sd%c", 'a' + channelNo * 2 + devNo);
            identifyDisk(hd);
            if (devNo != 0) {
                pno = 0;
                lno = 0;
                partitionScan(hd, 0);
            }
            devNo++;
        }
        channelNo++;
    }
    listTraversal(&partitionList, printPartitionInfo, 0);
    printk("ideInit done\n");
}

