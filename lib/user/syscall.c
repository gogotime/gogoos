#include "../stdint.h"
#include "../../kernel/user/syscall_define.h"
#include "../../fs/fs.h"


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

#define syscall2(number, arg1, arg2) ({         \
   int res;                                     \
   asm volatile (                               \
    "int $0x80;"                                \
    :"=a"(res)                                  \
    :"a"(number),"b"(arg1),"c"(arg2)            \
    :"memory");                                 \
    res;                                        \
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

void* malloc(uint32 size) {
    return (void*) syscall1(SYS_MALLOC, size);
}

void free(void* ptr) {
    syscall1(SYS_FREE, ptr);
}

int32 fork() {
    return syscall0(SYS_FORK);
}

uint32 read(int32 fd, const void* buf, uint32 count) {
    return syscall3(SYS_READ, fd, buf, count);
}

void putChar(char c) {
    syscall1(SYS_PUTCHAR, c);
}

void clear() {
    syscall0(SYS_CLEAR);
}

int32 open(const char* pathName, OFlags flags) {
    return syscall2(SYS_OPEN, pathName, flags);
}

int32 close(int32 fd) {
    return syscall1(SYS_CLOSE, fd);
}

int32 lseek(int32 fd, int32 offset, uint8 seekFlag) {
    return syscall3(SYS_LSEEK, fd, offset, seekFlag);
}

int32 unlink(const char* pathName) {
    return syscall1(SYS_UNLINK, pathName);
}

int32 mkdir(const char* pathName) {
    return syscall1(SYS_MKDIR, pathName);
}

char* getCwd(char* buf, uint32 size) {
    return (char*) syscall2(SYS_GETCWD, buf, size);
}

Dir* openDir(const char* pathName) {
    return (Dir*) syscall1(SYS_OPENDIR, pathName);
}

int32 closeDir(Dir* dir) {
    return syscall1(SYS_CLOSEDIR, dir);
}

int32 chdir(const char* pathName) {
    return syscall1(SYS_CHDIR, pathName);
}

int32 rmdir(const char* pathName){
    return syscall1(SYS_RMDIR, pathName);
}

DirEntry* readDir(Dir* dir){
    return (DirEntry*) syscall1(SYS_READDIR, dir);
}

void rewindDir(Dir* dir){
    syscall1(SYS_REWINDDIR, dir);
}

int32 stat(const char* path, Stat* stat){
    return syscall2(SYS_STAT, path, stat);
}

void ps(){
    syscall0(SYS_PS);
}

int32 execv(const char* path, char* argv[16]) {
    return syscall2(SYS_EXECV, path, argv);
}