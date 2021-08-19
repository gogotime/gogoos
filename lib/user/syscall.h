#ifndef __LIB_USER_SYSCALL_H
#define __LIB_USER_SYSCALL_H

uint32 getPid();

uint32 write(char* str);

void* malloc(uint32 size);

void free(void* ptr);

#endif