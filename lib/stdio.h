# ifndef __LIB_STDIO_H
# define __LIB_STDIO_H

#include "stdint.h"
#include <stdarg.h>

uint32 printf(const char* format, ...);

uint32 vsprintf(char* buf, const char* format, va_list ap);

# endif