#include "../lib/kernel/print.h"
#include "../lib/stdint.h"
#include "../lib/structure/bitmap.h"
#include "../lib/debug.h"
#include "../lib/string.h"
#include "global.h"
#include "memory.h"


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
//    while (1);

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
    uint32 pagePhyAddr = ((bitIdx * PG_SIZE) + pool->startAddr);
    return (void*) pagePhyAddr;
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
        uint32 pdePhyAddr = (uint32) palloc(&kRAP);
        *pde = (pdePhyAddr | PG_US_U | PG_RW_W | PG_P_1);
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
    lockLock(rpool->lock);

    void* vaddr = mallocPage(PF_USER, pageCnt);
    if (vaddr != NULL) {
        memset(vaddr, 0, pageCnt * PG_SIZE);
    }
    lockUnlock(rpool->lock);
    return vaddr;
}

void memInit() {
    putString("memInit start\n");
//    uint32 memBytesTotal = 33554432; //32M
    uint32 memBytesTotal = *((uint32*) (MEMORY_DATA_ADDR));
    putString("total memory bytes:");
    putUint32(memBytesTotal);
    putString("\n");
//    while (1);
    memPoolInit(memBytesTotal);
    putString("memInit done\n");
}

