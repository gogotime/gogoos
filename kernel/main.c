#include "../lib/kernel/print.h"
#include "../lib/kernel/io.h"
#include "../lib/kernel/interrupt.h"
#include "../lib/kernel/stdio.h"
#include "../lib/structure/bitmap.h"
#include "../lib/user/syscall.h"
#include "../lib/string.h"
#include "../lib/debug.h"
#include "../lib/stdio.h"

#include "../device/console.h"
#include "../device/ide.h"
#include "../device/keyboard.h"
#include "../device/timer.h"

#include "../fs/fs.h"

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

}

void testThread1(void* arg);

void userProcess(void* arg);

volatile uint32 cnt = 1;
extern Dir rootDir;
extern Partition* curPart;

int main() {
    initAll();
//    threadStart("thread1", 4, testThread1, "a");
//    threadStart("thread2", 4, testThread1, "a");
//    processStart(userProcess, "userproc1");
    enableIntr();
//    printDirEntry(&rootDir);
//    sysMkdir("/dir1/dir2");
//    sysMkdir("/dir1");
    Dir* root = sysOpenDir("/.");
    sysMkdir("/dir3");
    DirEntry* de = NULL;
    while ((de = sysReadDir(root)) != NULL) {
        printk("%d %s\n", de->fileType, de->fileName);
    }
    sysMkdir("/dir3/dir1");
    sysRmdir("/dir3");
    sysRmdir("/dir3/dir1");
    sysRmdir("/dir3");
    sysRewindDir(root);
    while ((de = sysReadDir(root)) != NULL) {
        printk("%d %s\n", de->fileType, de->fileName);
    }
//    printDirEntry(&rootDir);
//    Dir* dir1 = sysOpenDir("/dir1");
//    printDirEntry(dir1);

    while (1) {
//        threadBlock(TASK_BLOCKED);
//        consolePutString("Main ");
    };
    return 0;
}


void testThread1(void* arg) {
    void* addr1;
    void* addr2;
    void* addr3;
    void* addr4;
    void* addr5;
    void* addr6;
    int i = 1000;
    while (i--) {
        printk("user pid:%d i:%d\n", getPid(), i);
        void* addr1 = malloc(33);
        void* addr2 = malloc(64);
        void* addr3 = malloc(31);
        void* addr4 = malloc(17);
        printk("addr1:%x\naddr2:%x\naddr3:%x\naddr4:%x\n", addr1, addr2, addr3, addr4);
        free(addr1);
        free(addr2);
        free(addr3);
        free(addr4);
        void* addr5 = malloc(31);
        void* addr6 = malloc(31);
        void* addr7 = malloc(16);
        printk("addr5:%x\naddr6:%x\naddr7:%x", addr5, addr6, addr7);
        free(addr5);
        free(addr6);
        free(addr7);
    }
    while (1);
}

void userProcess(void* arg) {
    void* addr1;
    void* addr2;
    void* addr3;
    void* addr4;
    void* addr5;
    void* addr6;
    int i = 1000;
    while (i--) {
        printf("user pid:%d i:%d\n", getPid(), i);
        void* addr1 = malloc(33);
        void* addr2 = malloc(64);
        void* addr3 = malloc(31);
        void* addr4 = malloc(17);
        printf("addr1:%x\naddr2:%x\naddr3:%x\naddr4:%x\n", addr1, addr2, addr3, addr4);
        free(addr1);
        free(addr2);
        free(addr3);
        free(addr4);
        void* addr5 = malloc(31);
        void* addr6 = malloc(31);
        void* addr7 = malloc(16);
        printf("addr5:%x\naddr6:%x\naddr7:%x", addr5, addr6, addr7);
        free(addr5);
        free(addr6);
        free(addr7);
    }
    while (1);
}