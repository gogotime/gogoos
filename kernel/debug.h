#ifndef KERNEL_DEBUG_H
#define KERNEL_DEBUG_H

#include "../lib/stdint.h"

void panicAndSpin1(const char* fileName, uint32 line, const char* funcName);
void panicAndSpin2(const char* condition);

#define PANIC(VAR) panicAndSpin1(__FILE__,__LINE__,__func__); \
    panicAndSpin2(VAR)
#ifdef RELEASE
#define ASSERT(CONDITION) ((void)0)
#else
#define ASSERT(CONDITION) \
    if(CONDITION){}else{  \
        PANIC(#CONDITION);\
    }
#endif
#endif