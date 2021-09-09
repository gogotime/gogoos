#include "../lib/kernel/print.h"
#include "../lib/kernel/io.h"
#include "../lib/kernel/interrupt.h"
#include "../lib/kernel/stdio.h"
#include "../lib/kernel/asm/print.h"
#include "../lib/structure/bitmap.h"
#include "../lib/user/syscall.h"
#include "../lib/string.h"
#include "../lib/debug.h"
#include "../lib/user/stdio.h"

#include "../shell/shell.h"

#include "../device/console.h"
#include "../device/ide.h"
#include "../device/keyboard.h"
#include "../device/timer.h"

#include "../fs/fs.h"

#include "global.h"
#include "interrupt.h"
#include "memory.h"
#include "thread/thread.h"
#include "user/tss.h"
#include "user/process.h"
#include "user/syscall.h"

void initAll() {
    putString("init all\n");
    idtInit();
    syscallInit();
    memInit();
    timerInit();
    threadInit();
    consoleInit();
    keyBoardInit();
    tssInit();
    enableIntr();
    ideInit();
    fsInit();
    putString("init all done\n");
    disableIntr();
}

void testThread1(void* arg);

void userProcess(void* arg);

void init();

extern Dir rootDir;
extern Partition* curPart;
extern IDEChannel ideChannel[2];

int main() {
    initAll();
    uint32 fileSize = 9136;
    uint32 secCnt = DIV_ROUND_UP(fileSize, SECTOR_BYTE_SIZE);
    Disk* sda = &ideChannel[0].devices[0];
    void* buf = sysMalloc(secCnt * SECTOR_BYTE_SIZE);
    if (buf == NULL) {
        printk("main: buf = sysMalloc(secCnt * SECTOR_BYTE_SIZE) failed\n");
        while (1);
    }
    ideRead(sda, 400, buf, secCnt);
    int32 fd = sysOpen("/t1", O_CREAT | O_RDWR);

    if (fd == -1) {
        printk("create file test1 failed\n");
        while (1);
    }

    if (sysWrite(fd, buf, fileSize) == -1) {
        printk("write file test1 failed\n");
        while (1);
    }

    sysClose(fd);
    sysFree(buf);
    sysClear();
    processStart(init, "init");
    enableIntr();
    while (1);
    return 0;
}

void init() {
    uint32 pid = fork();
    if (pid) {
        int status;
        int childPid;
        while (1) {
            childPid = wait(&status);
            printf("init wait child pid %d status %d\n", childPid, status);
        }
    } else {
        myShell();
    }
    PANIC("shoule not be here")
}
