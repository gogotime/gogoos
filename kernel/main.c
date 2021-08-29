#include "../lib/kernel/print.h"
#include "../lib/kernel/io.h"
#include "../lib/kernel/interrupt.h"
#include "../lib/string.h"
#include "../lib/structure/bitmap.h"
#include "../lib/debug.h"
#include "../lib/user/syscall.h"
#include "../lib/stdio.h"
#include "../lib/kernel/stdio.h"

#include "../device/timer.h"
#include "../device/console.h"
#include "../device/keyboard.h"
#include "../device/ide.h"

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

typedef struct {
    uint16 limit;
    uint32 base;
} GDTR;

volatile uint32 cnt = 1;
extern Dir rootDir;
int main() {
    initAll();
//    threadStart("thread1", 4, testThread1, "a");
//    threadStart("thread2", 4, testThread1, "a");
//    processStart(userProcess, "userproc1");
    enableIntr();
    printDirEntry(&rootDir);
    uint32 fd=sysOpen("/file1", O_RDWR);
    printk("file1 ino:%d\n", fd);
    int32 res=sysClose(fd);
    if (res != -1) {
        printk("%d closed successfully\n", fd);
    }
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