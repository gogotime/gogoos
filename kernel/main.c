#include "../lib/kernel/print.h"
#include "../lib/kernel/io.h"
#include "../lib/kernel/interrupt.h"
#include "../lib/kernel/stdio.h"
#include "../lib/kernel/asm/print.h"
#include "../lib/structure/bitmap.h"
#include "../lib/user/syscall.h"
#include "../lib/string.h"
#include "../lib/debug.h"
#include "../lib/stdio.h"

#include "../shell/shell.h"

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
    disableIntr();
}

void testThread1(void* arg);

void userProcess(void* arg);

void init();

volatile uint32 cnt = 1;
extern Dir rootDir;
extern Partition* curPart;

int main() {
    initAll();
    sysClear();
//    threadStart("thread1", 4, testThread1, "a");
    processStart(init, "init");
    enableIntr();
    while (1);
    return 0;
}


void testThread1(void* arg) {
    while (1);
}

void init(){
    uint32 pid = fork();
    if (pid) {
        while (1);
    }else{
        myShell();
    }
}


void userProcess(void* arg) {
    int32 pid = fork();
    if (pid == -1) {
        printf("fork error\n");
    } else if (pid == 0) {
        printf("i'm child, my pid is %d\n", getPid());

    } else {
        printf("i'm father, my pid is %d, my child's pid is %d\n", getPid(),pid);
    }
    while (1);
}