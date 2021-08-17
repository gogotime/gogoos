#include "../stdint.h"

typedef enum {
    SYS_GETPID,
    SYS_WRITE
} SYSCALL;

#define syscall0(number) ({ \
   int res;                 \
   asm volatile (           \
    "int $0x80;"            \
    :"=a"(res)              \
    :"a"(number)            \
    :"memory");             \
    res;                    \
})

#define syscall1(number, arg1) ({ \
   int res;                      \
   asm volatile (                \
    "int $0x80;"                 \
    :"=a"(res)                   \
    :"a"(number),"b"(arg1)       \
    :"memory");                  \
    res;                         \
})

#define syscall3(number, arg1, arg2, arg3) ({   \
   int res;                                     \
   asm volatile (                               \
    "int $0x80;"                                \
    :"=a"(res)                                  \
    :"a"(number),"b"(arg1),"c"(arg2),"d"(arg3)  \
    :"memory");                                 \
    res;                                        \
})

uint32 getPid() {
    return syscall0(SYS_GETPID);
}


uint32 write(char* str) {
    return syscall1(SYS_WRITE, str);
}