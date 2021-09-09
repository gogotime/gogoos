#ifndef __LIB_USER_SYSCALL_H
#define __LIB_USER_SYSCALL_H

#include "../stdint.h"
#include "../../fs/fs.h"

uint32 getPid();

uint32 write(int32 fd, const void* buf, uint32 count);

void* malloc(uint32 size);

void free(void* ptr);

int32 fork();

uint32 read(int32 fd, const void* buf, uint32 count);

void putChar(char c);

void clear();

int32 open(const char* pathName, OFlags flags);

int32 close(int32 fd);

int32 lseek(int32 fd, int32 offset, uint8 seekFlag);

int32 unlink(const char* pathName);

int32 mkdir(const char* pathName);

char* getCwd(char* buf, uint32 size);

Dir* openDir(const char* pathName);

int32 closeDir(Dir* dir);

int32 chdir(const char* pathName);

int32 rmdir(const char* pathName);

DirEntry* readDir(Dir* dir);

void rewindDir(Dir* dir);

int32 stat(const char* path, Stat* stat);

void ps();

int32 execv(const char* path, char* argv[16]);

void exit(int32 status);

int32 wait(int32* status);

#endif