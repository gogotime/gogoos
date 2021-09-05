#ifndef LIB_KERNEL_ASM_PRINT_H
#define LIB_KERNEL_ASM_PRINT_H
#include "../../stdint.h"

void sysPutChar(char c);

void setCursor(uint16 pos);

void sysClear();
#endif
