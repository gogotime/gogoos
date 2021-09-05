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
} PhysicalAddrPool;


typedef enum {
    PF_USER,
    PF_KERNEL
} PoolFlag;





#define PG_P_1 1  //present
#define PG_P_0 0  //not present
#define PG_RW_R 0 //read execute
#define PG_RW_W 2 //read write execute
#define PG_US_S 0 //CPL=0,1,2
#define PG_US_U 4 //All CPL


void memInit();

uint32* getPtePtr(uint32 vaddr);

uint32* getPdePtr(uint32 vaddr);

void* allocPage(PoolFlag pf, uint32 pageCnt);

void freePage(PoolFlag pf, void* virPageAddr, uint32 pgCnt);

void* getKernelPages(uint32 pageCnt);

void* getUserPages(uint32 pageCnt);

void* getOnePage(PoolFlag pf, uint32 vaddr);

void* getOnePageWithoutVBM(PoolFlag pf, uint32 vaddr);

uint32 addrV2P(uint32 vaddr);

void memBlockDescInit(MemBlockDesc* descArr);

void* sysMalloc(uint32 size);

void sysFree(void* ptr);
# endif
