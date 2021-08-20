#include <stdarg.h>
#include "../stdio.h"
#include "../../device/console.h"
void printk(const char* format, ...) {
    va_list args;
    va_start(args, format);
    char buf[1024] = {0};
    vsprintf(buf, format, args);
    va_end(args);
    consolePutString(buf);
}

void sprintk(char* buf, const char* format, ...){
    va_list args;
    va_start(args, format);
    vsprintf(buf, format, args);
    va_end(args);
}