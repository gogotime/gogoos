#ifndef __KERNEL_USER_EXEC_H
#define __KERNEL_USER_EXEC_H

#include "../../lib/stdint.h"
#include "../thread/thread.h"

int32 sysExecv(const char* path, char* argv[16]) ;

#endif