#include "../lib/kernel/print.h"
#include "../lib/stdint.h"
#include "../lib/structure/bitmap.h"
#include "../lib/debug.h"
#include "../lib/string.h"
#include "global.h"
#include "memory.h"

typedef struct {
    MemBlockDesc desc;
    uint32 cnt;
    bool large;
} Arena;

MemBlockDesc kmbdArr[MEM_BLOCK_DESC_CNT];


RAddrPool kRAP, uRAP;
VAddrPool kVAP, uVAP;


static void memPoolInit(uint32 allMem) {
    putString("memPoolInit start\n");
    uint32 pageTableSize = PG_SIZE * 256;
    uint32 usedMem = pageTableSize + PAGE_DIR_TABLE_POS;
    uint32 freeMem = allMem - usedMem;
    uint32 allFreePages = freeMem / PG_SIZE;
    uint32 kernelFreePages = allFreePages / 2;
    uint32 userFreePages = allFreePages / 2;
    uint32 kbmLength = kernelFreePages / 8;
    uint32 ubmLength = userFreePages / 8;
    uint32 kernelPageStart = usedMem;
    uint32 userPageStart = kernelPageStart + kernelFreePages * PG_SIZE;
    kRAP.startAddr = kernelPageStart;
    uRAP.startAddr = userPageStart;

    kRAP.size = kernelFreePages * PG_SIZE;
    uRAP.size = userFreePages * PG_SIZE;

    kRAP.bitMap.length = kbmLength;
    uRAP.bitMap.length = ubmLength;

    kRAP.bitMap.startAddr = (uint8*) MEM_BITMAP_BASE;
    uRAP.bitMap.startAddr = (uint8*) (MEM_BITMAP_BASE + kbmLength);

    lockInit(&kRAP.lock);
    lockInit(&uRAP.lock);

    putString("kernelRAddrPool.bitMap.start=");
    putUint32Hex((uint32) kRAP.bitMap.startAddr);
    putString("\n");
    putString("kernelRAddrPool.startAddr=");
    putUint32Hex(kRAP.startAddr);
    putString("\n");
    putString("userRAddrPool.bitMap.start=");
    putUint32Hex((uint32) uRAP.bitMap.startAddr);
    putString("\n");
    putString("userRAddrPool.startAddr=");
    putUint32Hex(uRAP.startAddr);
    putString("\n");

    bitMapInit(&kRAP.bitMap);
    bitMapInit(&uRAP.bitMap);

    kVAP.bitMap.startAddr = (uint8*) (MEM_BITMAP_BASE + kbmLength + ubmLength);
    kVAP.bitMap.length = kbmLength;
    kVAP.startAddr = K_HEAP_START;
    bitMapInit(&kVAP.bitMap);
    putString("memPoolInit done");
}

static void* getVaddr(PoolFlag pf, uint32 pageCnt) {
    uint32 vaddrStart = (uint32) NULL;
    uint32 bitIdxStart = -1;
    uint32 cnt = 0;
    if (pf == PF_KERNEL) {
        bitIdxStart = bitMapScanAndSet(&kVAP.bitMap, pageCnt, 1);
        if (bitIdxStart == -1) {
            return NULL;
        }
        vaddrStart = kVAP.startAddr + bitIdxStart * PG_SIZE;
    } else {
        TaskStruct* cur = getCurrentThread();
        bitIdxStart = bitMapScanAndSet(&cur->vap.bitMap, pageCnt, 1);
        if (bitIdxStart == -1) {
            return NULL;
        }
        vaddrStart = cur->vap.startAddr + bitIdxStart * PG_SIZE;
        ASSERT((uint32) vaddrStart < (0xc0000000 - PG_SIZE))
    }
    return (void*) vaddrStart;
}

uint32* getPtePtr(uint32 vaddr) {
    uint32* pte = (uint32*) (0xffc00000 + ((vaddr & 0xffc00000) >> 10) + PTE_IDX(vaddr) * 4);
    return pte;
}

uint32* getPdePtr(uint32 vaddr) {
    uint32* pde = (uint32*) (0xfffff000 + PDE_IDX(vaddr) * 4);
    return pde;
}

static void* palloc(RAddrPool* pool) {
    int bitIdx = bitMapScan(&pool->bitMap, 1);
    if (bitIdx == -1) {
        return NULL;
    }
    bitMapSet(&pool->bitMap, bitIdx, 1);
    uint32 pageRaddr = ((bitIdx * PG_SIZE) + pool->startAddr);
    return (void*) pageRaddr;
}

static void mapPageTable(void* vaddr, void* paddr) {
    uint32 va = (uint32) vaddr;
    uint32 pa = (uint32) paddr;
    uint32* pde = getPdePtr(va);
    uint32* pte = getPtePtr(va);
    if (*pde & 0x00000001) {
        if (!(*pte & 0x00000001)) {
            *pte = (pa | PG_US_U | PG_RW_W | PG_P_1);
        } else {
            PANIC("pte repeat")
        }
    } else {
        uint32 pageRaddr = (uint32) palloc(&kRAP);
        *pde = (pageRaddr | PG_US_U | PG_RW_W | PG_P_1);
        memset((void*) ((uint32) pte & 0xfffff000), 0, PG_SIZE);
        *pte = (pa | PG_US_U | PG_RW_W | PG_P_1);
    }

}

void* mallocPage(PoolFlag pf, uint32 pageCnt) {
    ASSERT(pageCnt > 0 && pageCnt < 3840)
    void* vaddrStart = getVaddr(pf, pageCnt);
    if (vaddrStart == NULL) {
        return NULL;
    }
    uint32 vaddr = (uint32) vaddrStart;
    uint32 cnt = pageCnt;
    RAddrPool* rap = pf == PF_KERNEL ? &kRAP : &uRAP;
    while (cnt-- > 0) {
        void* paddr = palloc(rap);
        if (paddr == NULL) {
            // TODO rollback if failed
            return NULL;
        }
        mapPageTable((void*) vaddr, paddr);
        vaddr += PG_SIZE;
    }
    return vaddrStart;
}

void* getKernelPages(uint32 pageCnt) {
    lockLock(&kRAP.lock);
    void* vaddr = mallocPage(PF_KERNEL, pageCnt);
    if (vaddr != NULL) {
        memset(vaddr, 0, pageCnt * PG_SIZE);
    }
    lockUnlock(&kRAP.lock);
    return vaddr;
}

void* getUserPages(uint32 pageCnt) {
    lockLock(&uRAP.lock);
    void* vaddr = mallocPage(PF_USER, pageCnt);
    if (vaddr != NULL) {
        memset(vaddr, 0, pageCnt * PG_SIZE);
    }
    lockUnlock(&uRAP.lock);
    return vaddr;
}

void* getOnePage(PoolFlag pf, uint32 vaddr) {
    RAddrPool* rpool = pf == PF_KERNEL ? &kRAP : &uRAP;
    lockLock(&rpool->lock);
    int32 bitIdx = -1;
    TaskStruct* cur = getCurrentThread();
    if (pf == PF_USER && cur->pageDir != NULL) {
        bitIdx = (vaddr - cur->vap.startAddr) / PG_SIZE;
        ASSERT(bitIdx >= 0)
        bitMapSet(&cur->vap.bitMap, bitIdx, 1);
    } else if (pf == PF_KERNEL && cur->pageDir == NULL) {
        bitIdx = (vaddr - kVAP.startAddr) / PG_SIZE;
        ASSERT(bitIdx >= 0)
        bitMapSet(&kVAP.bitMap, bitIdx, 1);
    } else {
        putString("\n");
        putString("PoolFlag:");
        putUint32(pf);
        putString("\ncur->pageDir:");
        putUint32Hex((uint32) cur->pageDir);
        PANIC("PoolFlag and Vaddr not matched\n")
    }
    void* raddr = palloc(rpool);
    if (raddr == NULL) {
        return NULL;
    }
    mapPageTable((void*) vaddr, raddr);
    lockUnlock(&rpool->lock);
    return (void*) vaddr;
}

uint32 addrV2P(uint32 vaddr) {
    uint32* pte = getPtePtr(vaddr);
    return ((*pte & 0xfffff000) + (vaddr & 0x00000fff));
}

void memBlockDescInit(MemBlockDesc* descArr) {
    uint16 descIdx = 0;
    uint32 blockSize = 16;
    for (; descIdx < MEM_BLOCK_DESC_CNT; descIdx++) {
        descArr[descIdx].blockSize = blockSize;
        descArr[descIdx].blocksPerArena = (PG_SIZE - sizeof(Arena)) / blockSize;
        listInit(&descArr[descIdx].freeList);
        blockSize *= 2;
    }
}

static MemBlock* arenaToBlock(Arena* a, uint32 idx) {
    return (MemBlock*) ((uint32) a + sizeof(Arena) + idx * a->desc.blockSize);
}

static Arena* blockToArena(MemBlock* b) {
    return (Arena*) ((uint32) b & 0xfffff000);
}

void* sysMalloc(uint32 size) {
    PoolFlag pf;
    RAddrPool* rap;
    uint32 poolSize;
    MemBlockDesc* mbdArr;
    TaskStruct* cur = getCurrentThread();
    if (cur->pageDir == NULL) {
        pf = PF_KERNEL;
        poolSize = kRAP.size;
        rap = &kRAP;
        mbdArr = kmbdArr;
    } else {
        pf = PF_USER;
        poolSize = uRAP.size;
        rap = &uRAP;
        mbdArr = cur->umbdArr;
    }

    if (!(size > 0 && size < poolSize)) {
        return NULL;
    }

    Arena* arena;
    MemBlock* block;
    lockLock(&rap->lock);

    if (size > 1024) {
        uint32 pgCnt = DIV_ROUND_UP(size + sizeof(Arena), PG_SIZE);
        arena = mallocPage(pf, pgCnt);
        if (arena != NULL) {
            arena->desc = NULL;
            arena->cnt = pgCnt;
            arena->large = true;
            lockUnlock(&rap->lock);
            return (void*) (arena + 1);
        } else {
            lockUnlock(&rap->lock);
            return NULL;
        }
    } else {

    }

}

void memInit() {
    putString("memInit start\n");
    uint32 memBytesTotal = *((uint32*) (MEMORY_DATA_ADDR));
    putString("total memory bytes:");
    putUint32(memBytesTotal);
    putString("\n");
    memPoolInit(memBytesTotal);
    memBlockDescInit(kmbdArr);
    putString("memInit done\n");
}

