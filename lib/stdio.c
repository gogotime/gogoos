#include "string.h"
#include "stdint.h"
#include "user/syscall.h"


//#define va_start(ap, v) ap=(va_list)&v
#ifndef _VA_LIST
typedef __builtin_va_list va_list;
#define _VA_LIST
#endif
#define va_start(ap, param) __builtin_va_start(ap, param)
#define va_end(ap)          __builtin_va_end(ap)
#define va_arg(ap, type)    __builtin_va_arg(ap, type)


static void itoa(uint32 val, char** bufPtrAddr) {
    if (val == 0) {
        *((*bufPtrAddr)++) = '0';
        return;
    }
    int8 idx = 0;
    char remain;
    char buff[10] = {0};
    while (val) {
        remain = val % 10;
        val = val / 10;
        buff[idx] = remain + '0';
        idx++;
    }
    idx--;
    while (idx >= 0) {
        *((*bufPtrAddr)++) = buff[idx];
        idx--;
    }
}

static void itoha(uint32 val, char** bufPtrAddr) {
    if (val == 0) {
        *((*bufPtrAddr)++) = '0';
        *((*bufPtrAddr)++) = 'x';
        *((*bufPtrAddr)++) = '0';
        return;
    }
    int8 idx = 0;
    char remain;
    char buff[12] = {0};
    while (val) {
        remain = val % 16;
        val = val / 16;
        buff[idx] = remain + '0';
        idx++;
    }
    buff[idx] = 'x';
    idx++;
    buff[idx] = '0';
    while (idx >= 0) {
        if (buff[idx] > '9' && buff[idx] != 'x') {
            buff[idx] += 7;
        }
        *((*bufPtrAddr)++) = buff[idx];
        idx--;
    }
}


uint32 vsprintf(char* buf, const char* format, va_list ap) {
    char* bufPtr = buf;
    const char* fmtPtr = format;
    char curChar = *format;
    int32 argInt = 0;
    char* argStr;
    while (curChar) {
        if (curChar != '%') {
            *(bufPtr++) = curChar;
            curChar = *(++fmtPtr);
            continue;
        }
        curChar = *(++fmtPtr);
        switch (curChar) {
            case 'd':
                argInt = va_arg(ap, int32);
                if (argInt < 0) {
                    *(bufPtr++) = '-';
                    argInt = 0 - argInt;
                }
                itoa(argInt, &bufPtr);
                break;
            case 'x':
                argInt = va_arg(ap, uint32);
                itoha(argInt, &bufPtr);
                break;
            case 'c':
                *(bufPtr++) = va_arg(ap, int);
                break;
            case 's':
                argStr = va_arg(ap, char *);
                strcpy(bufPtr, argStr);
                bufPtr += strlen(argStr);
                break;
            default:
                *(bufPtr++) = curChar;
        }
        curChar = *(++fmtPtr);
    }
    return strlen(buf);
};

uint32 printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    char buf[1024] = {0};
    vsprintf(buf, format, args);
    va_end(args);
    return write(1, buf, strlen(buf));
}