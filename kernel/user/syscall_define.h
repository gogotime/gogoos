#ifndef __KERNEL_USER_SYSCALL_DEFINE_H
#define __KERNEL_USER_SYSCALL_DEFINE_H

typedef enum {
    SYS_GETPID,
    SYS_WRITE,
    SYS_MALLOC,
    SYS_FREE,
    SYS_FORK
} SYSCALL;

#endif
