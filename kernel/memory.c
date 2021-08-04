#include "../lib/kernel/print.h"
#include "../lib/stdint.h"
#include "../lib/structure/bitmap.h"

#define PG_SIZE 4096 //4K
#define MEM_BITMAP_BASE 0xc008a000

#define K_HEAP_START 0xc0100000

typedef struct {
    BitMap bm;
    uint32 phyAddrStart;
    uint32 size;
} Pool;

Pool kernelPool,userPool

static void memPoolInit(uint32 allMem){
    putString("memPoolInit start\n");
    uint32 pageTableSize = PG_SIZE * 256;
    uint32 usedMem=pageTableSize+0x100000;
    uint32 freeMem = allMem - usedMem;
    uint32 allFreePages = freeMem / PG_SIZE;

};