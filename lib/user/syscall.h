#ifndef __LIB_USER_SYSCALL_H
#define __LIB_USER_SYSCALL_H

uint32 getPid();

uint32 write(int32 fd, const void* buf, uint32 count);

void* malloc(uint32 size);

void free(void* ptr);

int32 fork();

#endif