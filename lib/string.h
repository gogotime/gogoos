# ifndef __LIB_STRING_H
# define __LIB_STRING_H

#include "stdint.h"

void memset(void* dst, uint8 val, uint32 size);

void memcpy(void* dst, const void* src, uint32 size);

int memcmp(const void* s1, const void* s2, uint32 size);

char* strcpy(char* dst, const char* src);

uint32 strlen(const char* str);

int32 strcmp(const char* s1, const char* s2);

char* strchr(const char* str, const char c);

char* strrchr(const char* str, const char c);

char* strcat(char* dst, const char* src);

uint32 strchrs(const char* str, const char c);

# endif