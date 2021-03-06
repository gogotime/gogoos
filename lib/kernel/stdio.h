# ifndef __LIB_STDIO_KERNEL_H
# define __LIB_STDIO_KERNEL_H


#include <stdarg.h>

void printk(const char* format, ...);

void sprintk(char* buf, const char* format, ...);

# endif