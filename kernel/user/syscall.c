#include "../thread/thread.h"
#include "../memory.h"
#include "../../fs/fs.h"
#include "../../lib/stdint.h"
#include "../../lib/kernel/print.h"
#include "../../device/console.h"
#include "syscall_define.h"
#define SYSCALL_NUMBER 32

typedef uint32 syscallHandler;
syscallHandler syscallHandlerTable[SYSCALL_NUMBER];


void syscallInit() {
    putString("syscallInit start");
    syscallHandlerTable[SYS_GETPID] = (uint32) &sysGetPid;
    syscallHandlerTable[SYS_WRITE] = (uint32) &sysWrite;
    syscallHandlerTable[SYS_MALLOC] = (uint32) &sysMalloc;
    syscallHandlerTable[SYS_FREE] = (uint32) &sysFree;
    putString("syscallInit done");
}