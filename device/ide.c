#include "ide.h"
#include "../lib/kernel/stdio.h"
#include "../lib/debug.h"
#include "../kernel/global.h"
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

#define BIT_ALT_STAT_BSY 0x80
#define BIT_ALT_STAT_DRDY 0x40
#define BIT_ALT_STAT_DRQ 0x8

#define BIT_DEV_MBS 0xa0
#define BIT_DEV_LBA 0x40
#define BIT_DEV_DEV 0x10

#define CMD_IDENTIFY 0xec
#define CMD_READ_SECTOR 0x20
#define CMD_WRITE_SECTOR 0x30

#define MAX_LBA ((80*1024*1024/512)-1)

uint8 channelCnt;
uint8 hdCnt;
IDEChannel ideChannel[2];

void ideInit() {
    printk("ideInit start\n");
    hdCnt = *((uint8*) 0x475);
    ASSERT(hdCnt>0)
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
        channelNo++;
    }
    printk("ideInit end\n");
}