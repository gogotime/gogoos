#ifndef __LIB_KERNEL_PRINT_H
#define __LIB_KERNEL_PRINT_H

#include "../stdint.h"
#include "asm/print.h"

void putString(char* string);

void putUint32(uint32 val);

void putUint32Hex(uint32 val);

#endif