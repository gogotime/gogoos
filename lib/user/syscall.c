#include "../stdint.h"
#include "../../kernel/user/syscall_define.h"


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


uint32 write(int32 fd, const void* buf, uint32 count) {
    return syscall3(SYS_WRITE, fd, buf, count);
}

uint32 read(int32 fd, const void* buf, uint32 count) {
    return syscall3(SYS_READ, fd, buf, count);
}

void* malloc(uint32 size) {
    return (void*) syscall1(SYS_MALLOC, size);
}

void free(void* ptr) {
    syscall1(SYS_FREE, ptr);
}

int32 fork() {
    return syscall0(SYS_FORK);
}

void clear(){
    syscall0(SYS_CLEAR);
}

void putChar(char c){
    syscall1(SYS_PUTCHAR, c);
}