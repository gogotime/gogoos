#ifndef __KERNEL_USER_WAITEXIT_H
#define __KERNEL_USER_WAITEXIT_H

#include "../../lib/stdint.h"
#include "../thread/thread.h"

int32 sysWait(int32* status);

void sysExit(int32 status);

#endif