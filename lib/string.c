#include "string.h"
#include "stdint.h"
#include "kernel/print.h"
void memset(void* dst, uint8 val, uint32 size) {
    uint8* d = (uint8*) dst;
    while (size--) {
        *d++ = val;
    }
}

void memcpy(void* dst, const void* src, uint32 size) {
    uint8* d = (uint8*) dst;
    uint8* s = (uint8*) src;
    while (size--) {
        *d++ = *s++;
    }
}

int memcmp(const void* s1, const void* s2, uint32 size) {
    uint8* ss1 = (uint8*) s1;
    uint8* ss2 = (uint8*) s2;
    while (size--) {
        if (*ss1 != *ss2) {
            return *ss1 > *ss2 ? 1 : -1;
        }
        ss1++;
        ss2++;
    }
    return 0;
}

char* strcpy(char* dst, const char* src) {
    char* r = dst;
    while ((*dst++ = *src++));
    return r;
}

uint32 strlen(const char* str) {
    uint32 size = 0;
    while (*str++) {
        size++;
    }
    return size;
}

int32 strcmp(const char* s1, const char* s2) {
    while (*s1 != 0 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return *s1 < *s2 ? -1 : *s1 > *s2;
}

// return the first pointer of char c in string str
// if it doesn't exist, return NULL
char* strchr(const char* str, const char c) {
    while (*str != 0) {
        if (*str == c) {
            return (char*) str;
        }
        str++;
    }
    return NULL;
}

// Returns the pointer to the first occurrence of c in str
// if it doesn't exist, return NULL
char* strrchr(const char* str, const char c) {
    const char* lc = NULL;
    while (*str != 0) {
        if (*str == c) {
            lc = str;
        }
        str++;
    }
    return (char*) lc;
}

char* strcat(char* dst, const char* src) {
    char* str = dst;
    while (*str++);
    --str;
    while ((*str++ = *src++));
    return dst;
}

// Returns the number of occurrences of c in str
uint32 strchrs(const char* str, const char c) {
    uint32 cnt = 0;
    while (*str != 0) {
        if (*str == c) {
            cnt++;
        }
        str++;
    }
    return cnt;
}