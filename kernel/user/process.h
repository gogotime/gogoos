#ifndef __KERNEL_USER_PROCESS_H
#define __KERNEL_USER_PROCESS_H

#include "../../lib/stdint.h"

void processExecute(void* fileName);

void pageDirActivate(TaskStruct* ts);

void processActivate(TaskStruct* ts);

uint32* pageDirCreate(void);

void userVaddrBitMapInit(TaskStruct* userTs);

void processStart(void* fileName, char* name);


#endif