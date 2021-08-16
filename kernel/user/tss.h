#ifndef __KERNEL_USER_TSS_H
#define __KERNEL_USER_TSS_H

#include "../../lib/stdint.h"
#include "../thread/thread.h"

void updateTSSEsp(TaskStruct* ts);

void tssInit();

#endif