#ifndef __KERNEL_TSS_H
#define __KERNEL_TSS_H

#include "../lib/stdint.h"
#include "thread/thread.h"

void updateTSSEsp(TaskStruct* ts);

void tssInit();

#endif