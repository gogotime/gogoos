#ifndef __KERNEL_INTERRUPT_H
#define __KERNEL_INTERRUPT_H

#include "../lib/stdint.h"

typedef void (*IntrHandler)(uint8 intrNo);

void idtInit();

void registerIntrHandler(uint8 intrNo,IntrHandler handler);



#endif