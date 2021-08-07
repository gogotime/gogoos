#ifndef __LIB_KERNEL_INTERRUPT_H
#define __LIB_KERNEL_INTERRUPT_H

#include "../lib/kernel/interrupt.h"

void idtInit();

void registerIntrHandler(uint8 intrNo,IntrHandler handler)

#endif