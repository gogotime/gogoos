#include "ide.h"
#include "timer.h"
#include "../lib/kernel/stdio.h"
#include "../lib/kernel/io.h"
#include "../lib/debug.h"
#include "../kernel/global.h"
#include "../kernel/interrupt.h"

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

typedef struct partitionTableEntry {
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
} PartitionTableEntry;

typedef struct bootSector {
    uint8 other[446];
    PartitionTableEntry partitionTable[4];
    uint16 signature;
} BootSector;

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
    while (timeLimit -= 10 >= 0) {
        if (!inb(regStatus(channel) & BIT_STAT_BSY)) {
            return inb(regStatus(channel)) & BIT_STAT_DRQ;
        } else {
            sleepMs(10);
        }
    }
    return false;
}

void ideRead(Disk* hd, uint32 lba, void* buf, uint32 secCnt) {
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
        sprintk(error, "%s identify %d failed!!!\n", hd->name, lba);
        PANIC(error)
    }
    readSector(hd, info, 1);

    char buf[64];


}

void ideInit() {
    printk("ideInit start\n");
    hdCnt = *((uint8*) 0x475);
    ASSERT(hdCnt > 0)
    channelCnt = DIV_ROUND_UP(hdCnt, 2);
    IDEChannel* channel;
    uint8 channelNo = 0;
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
        channelNo++;
    }
    printk("ideInit end\n");
}

