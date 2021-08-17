#ifndef __KERNEL_USER_SYSCALL_H
#define __KERNEL_USER_SYSCALL_H


uint32 sysWrite(char* str);

uint32 sysGetPid();

void syscallInit();

#endif