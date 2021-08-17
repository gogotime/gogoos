#include "../thread/thread.h"
#include "../../lib/stdint.h"
#include "../../lib/kernel/print.h"
#include "../../device/console.h"

#define SYSCALL_NUMBER 32

typedef uint32 syscallHandler;
syscallHandler syscallHandlerTable[SYSCALL_NUMBER];

typedef enum {
    SYS_GETPID,
    SYS_WRITE
} SYSCALL;

uint32 sysGetPid() {
    return getCurrentThread()->pid;
}

uint32 sysWrite(char* str) {
    consolePutString(str);
    return 0;
}

void syscallInit() {
    putString("syscallInit start");
    syscallHandlerTable[SYS_GETPID] = (uint32) &sysGetPid;
    syscallHandlerTable[SYS_WRITE] = (uint32) &sysWrite;
    putString("syscallInit done");
}