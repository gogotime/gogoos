#include "../thread/thread.h"
#include "../memory.h"
#include "fork.h"
#include "exec.h"
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
    syscallHandlerTable[SYS_FORK] = (uint32) &sysFork;
    syscallHandlerTable[SYS_READ] = (uint32) &sysRead;
    syscallHandlerTable[SYS_PUTCHAR] = (uint32) &sysPutChar;
    syscallHandlerTable[SYS_CLEAR] = (uint32) &sysClear;
    syscallHandlerTable[SYS_OPEN] = (uint32) &sysOpen;
    syscallHandlerTable[SYS_CLOSE] = (uint32) &sysClose;
    syscallHandlerTable[SYS_LSEEK] = (uint32) &sysLseek;
    syscallHandlerTable[SYS_UNLINK] = (uint32) &sysUnlink;
    syscallHandlerTable[SYS_MKDIR] = (uint32) &sysMkdir;
    syscallHandlerTable[SYS_GETCWD] = (uint32) &sysGetCwd;
    syscallHandlerTable[SYS_OPENDIR] = (uint32) &sysOpenDir;
    syscallHandlerTable[SYS_CLOSEDIR] = (uint32) &sysCloseDir;
    syscallHandlerTable[SYS_CHDIR] = (uint32) &sysChDir;
    syscallHandlerTable[SYS_RMDIR] = (uint32) &sysRmdir;
    syscallHandlerTable[SYS_READDIR] = (uint32) &sysReadDir;
    syscallHandlerTable[SYS_REWINDDIR] = (uint32) &sysRewindDir;
    syscallHandlerTable[SYS_STAT] = (uint32) &sysStat;
    syscallHandlerTable[SYS_PS] = (uint32) &sysPs;
    syscallHandlerTable[SYS_EXECV] = (uint32) &sysExecv;
    putString("syscallInit done");
}