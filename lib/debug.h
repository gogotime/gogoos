#ifndef __LIB_DEBUG_H
#define __LIB_DEBUG_H

#include "stdint.h"


void panicAndSpin1(const char* fileName, uint32 line, const char* funcName, const char* condition);

#define PANIC(VAR) panicAndSpin1(__FILE__,__LINE__,__func__,VAR);
#ifdef RELEASE
#define ASSERT(CONDITION) ((void)0)
#else
#define ASSERT(CONDITION) \
    if(CONDITION){}else{  \
        PANIC(#CONDITION);\
    }
#endif
#endif