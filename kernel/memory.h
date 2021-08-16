# ifndef __KERNEL_MEMORY_H
# define __KERNEL_MEMORY_H


#include "thread/sync.h"
#include "thread/thread.h"
#include "../lib/structure/bitmap.h"
typedef struct {
    BitMap bitMap;
    uint32 startAddr;
    uint32 size;
    Lock lock;
} RAddrPool;


typedef enum  {
    PF_USER,
    PF_KERNEL
}PoolFlag;

#define PG_P_1 1  //present
#define PG_P_0 0  //not present
#define PG_RW_R 0 //read execute
#define PG_RW_W 2 //read write execute
#define PG_US_S 0 //CPL=0,1,2
#define PG_US_U 4 //All CPL


void memInit();

uint32* getPtePtr(uint32 vaddr);

uint32* getPdePtr(uint32 vaddr);

void* mallocPage(PoolFlag pf, uint32 pageCnt);

void* getKernelPages(uint32 pageCnt);

# endif
