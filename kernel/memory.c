#include "../lib/kernel/print.h"
#include "../lib/stdint.h"
#include "../lib/structure/bitmap.h"
#include "../lib/debug.h"
#include "../lib/string.h"
#include "global.h"
#include "memory.h"

typedef struct {
    MemBlockDesc* desc;
    uint32 cnt;
    bool large;
} Arena;

MemBlockDesc kmbdArr[MEM_BLOCK_DESC_CNT];


PhysicalAddrPool kernelPap, userPap;
VirtualAddrPool kernelVap;


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
    kernelPap.startAddr = kernelPageStart;
    userPap.startAddr = userPageStart;

    kernelPap.size = kernelFreePages * PG_SIZE;
    userPap.size = userFreePages * PG_SIZE;

    kernelPap.bitMap.length = kbmLength;
    userPap.bitMap.length = ubmLength;

    kernelPap.bitMap.startAddr = (uint8*) MEM_BITMAP_BASE;
    userPap.bitMap.startAddr = (uint8*) (MEM_BITMAP_BASE + kbmLength);

    lockInit(&kernelPap.lock);
    lockInit(&userPap.lock);

    putString("kernelPhysicalAddrPool.bitMap.start=");
    putUint32Hex((uint32) kernelPap.bitMap.startAddr);
    putString("\n");
    putString("kernelPhysicalAddrPool.startAddr=");
    putUint32Hex(kernelPap.startAddr);
    putString("\n");
    putString("userPhysicalAddrPool.bitMap.start=");
    putUint32Hex((uint32) userPap.bitMap.startAddr);
    putString("\n");
    putString("userPhysicalAddrPool.startAddr=");
    putUint32Hex(userPap.startAddr);
    putString("\n");

    bitMapInit(&kernelPap.bitMap);
    bitMapInit(&userPap.bitMap);

    kernelVap.bitMap.startAddr = (uint8*) (MEM_BITMAP_BASE + kbmLength + ubmLength);
    kernelVap.bitMap.length = kbmLength;
    kernelVap.startAddr = K_HEAP_START;
    bitMapInit(&kernelVap.bitMap);
    putString("memPoolInit done\n");
}

uint32* getPtePtr(uint32 vaddr) {
    uint32* pte = (uint32*) (0xffc00000 + ((vaddr & 0xffc00000) >> 10) + PTE_IDX(vaddr) * 4);
    return pte;
}

uint32* getPdePtr(uint32 vaddr) {
    uint32* pde = (uint32*) (0xfffff000 + PDE_IDX(vaddr) * 4);
    return pde;
}

uint32 addrV2P(uint32 vaddr) {
    uint32* pte = getPtePtr(vaddr);
    return ((*pte & 0xfffff000) + (vaddr & 0x00000fff));
}

static MemBlock* arenaToBlock(Arena* a, uint32 idx) {
    return (MemBlock*) ((uint32) a + sizeof(Arena) + idx * a->desc->blockSize);
}

static Arena* blockToArena(MemBlock* b) {
    return (Arena*) ((uint32) b & 0xfffff000);
}



static void* allocPhyAddr(PhysicalAddrPool* pap) {
    int bitIdx = bitMapScan(&pap->bitMap, 1);
    if (bitIdx == -1) {
        return NULL;
    }
    bitMapSet(&pap->bitMap, bitIdx, 1);
    uint32 phyPageAddr = ((bitIdx * PG_SIZE) + pap->startAddr);
    return (void*) phyPageAddr;
}

void freePhyAddr(uint32 phyPageAddr) {
    PhysicalAddrPool* pap;
    uint32 bitIdx = 0;
    if (phyPageAddr >= userPap.startAddr) {
        pap = &userPap;
    } else {
        pap = &kernelPap;
    }
    bitIdx = (phyPageAddr - pap->startAddr) / PG_SIZE;
    bitMapSet(&pap->bitMap, bitIdx, 0);
}

static void* allocVirAddr(PoolFlag pf, uint32 pageCnt) {
    uint32 virPageAddr = (uint32) NULL;
    uint32 bitIdxStart = -1;
    uint32 cnt = 0;
    if (pf == PF_KERNEL) {
        bitIdxStart = bitMapScanAndSet(&kernelVap.bitMap, pageCnt, 1);
        if (bitIdxStart == -1) {
            return NULL;
        }
        virPageAddr = kernelVap.startAddr + bitIdxStart * PG_SIZE;
    } else {
        TaskStruct* cur = getCurrentThread();
        bitIdxStart = bitMapScanAndSet(&cur->vap.bitMap, pageCnt, 1);
        if (bitIdxStart == -1) {
            return NULL;
        }
        virPageAddr = cur->vap.startAddr + bitIdxStart * PG_SIZE;
        ASSERT((uint32) virPageAddr < (0xc0000000 - PG_SIZE))
    }
    return (void*) virPageAddr;
}

static void freeVirAddr(PoolFlag pf, void* virPageAddr, uint32 pgCnt) {
    uint32 bitIdxStart = 0;
    uint32 vaddr = (uint32) virPageAddr;
    uint32 cnt = 0;
    if (pf == PF_KERNEL) {
        bitIdxStart = (vaddr - kernelVap.startAddr) / PG_SIZE;
        while (cnt < pgCnt) {
            bitMapSet(&kernelVap.bitMap, bitIdxStart + cnt, 0);
            cnt++;
        }
    } else {
        TaskStruct* cur = getCurrentThread();
        bitIdxStart = (vaddr - cur->vap.startAddr) / PG_SIZE;
        while (cnt < pgCnt) {
            bitMapSet(&cur->vap.bitMap, bitIdxStart + cnt, 0);
            cnt++;
        }
    }
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
        uint32 phyPageAddr = (uint32) allocPhyAddr(&kernelPap);
        *pde = (phyPageAddr | PG_US_U | PG_RW_W | PG_P_1);
        memset((void*) ((uint32) pte & 0xfffff000), 0, PG_SIZE);
        *pte = (pa | PG_US_U | PG_RW_W | PG_P_1);
    }

}

static void unmapPageTable(uint32 virPageAddr) {
    uint32* pte = getPtePtr(virPageAddr);
    *pte &= ~PG_P_1;
    asm volatile("invlpg %0;"::"m"(virPageAddr):"memory");
}

void* allocPage(PoolFlag pf, uint32 pageCnt) {
    ASSERT(pageCnt > 0 && pageCnt < 3840)
    void* virPageAddr = allocVirAddr(pf, pageCnt);
    if (virPageAddr == NULL) {
        return NULL;
    }
    uint32 vaddr = (uint32) virPageAddr;
    uint32 cnt = pageCnt;
    PhysicalAddrPool* pap = pf == PF_KERNEL ? &kernelPap : &userPap;
    while (cnt-- > 0) {
        void* paddr = allocPhyAddr(pap);
        if (paddr == NULL) {
            vaddr -= PG_SIZE;
            cnt++;
            while (cnt < pageCnt) {
                freePhyAddr(addrV2P(vaddr));
                unmapPageTable(vaddr);
                vaddr -= PG_SIZE;
                cnt++;
            }
            freeVirAddr(pf, virPageAddr, pageCnt);
            ASSERT((uint32) vaddr < (uint32) virPageAddr)
            return NULL;
        }
        mapPageTable((void*) vaddr, paddr);
        vaddr += PG_SIZE;
    }
    return virPageAddr;
}

void freePage(PoolFlag pf, void* virPageAddr, uint32 pgCnt) {
    uint32 paddr;
    uint32 vaddr = (uint32) virPageAddr;
    uint32 cnt = 0;
    ASSERT(pgCnt > 0 && vaddr % PG_SIZE == 0)
    paddr = addrV2P(vaddr);
    ASSERT((paddr % PG_SIZE) == 0 && paddr > 0x102000)
    if (paddr >= userPap.startAddr) {
        while (cnt < pgCnt) {
            paddr = addrV2P(vaddr);
            ASSERT((paddr % PG_SIZE) == 0 && paddr > userPap.startAddr)
            freePhyAddr(paddr);
            unmapPageTable(vaddr);
            vaddr++;
            cnt++;
        }
        freeVirAddr(pf, virPageAddr, pgCnt);
    } else {
        while (cnt < pgCnt) {
            paddr = addrV2P(vaddr);
            ASSERT((paddr % PG_SIZE) == 0 && paddr >= kernelPap.startAddr && paddr < userPap.startAddr)
            freePhyAddr(paddr);
            unmapPageTable(vaddr);
            vaddr++;
            cnt++;
        }
        freeVirAddr(pf, virPageAddr, pgCnt);
    }
}


void* getKernelPages(uint32 pageCnt) {
    lockLock(&kernelPap.lock);
    void* vaddr = allocPage(PF_KERNEL, pageCnt);
    if (vaddr != NULL) {
        memset(vaddr, 0, pageCnt * PG_SIZE);
    }
    lockUnlock(&kernelPap.lock);
    return vaddr;
}

void* getUserPages(uint32 pageCnt) {
    lockLock(&userPap.lock);
    void* vaddr = allocPage(PF_USER, pageCnt);
    if (vaddr != NULL) {
        memset(vaddr, 0, pageCnt * PG_SIZE);
    }
    lockUnlock(&userPap.lock);
    return vaddr;
}

void* getOnePage(PoolFlag pf, uint32 vaddr) {
    PhysicalAddrPool* pap = pf == PF_KERNEL ? &kernelPap : &userPap;
    lockLock(&pap->lock);
    int32 bitIdx = -1;
    TaskStruct* cur = getCurrentThread();
    if (pf == PF_USER && cur->pageDir != NULL) {
        bitIdx = (vaddr - cur->vap.startAddr) / PG_SIZE;
        ASSERT(bitIdx >= 0)
        bitMapSet(&cur->vap.bitMap, bitIdx, 1);
    } else if (pf == PF_KERNEL && cur->pageDir == NULL) {
        bitIdx = (vaddr - kernelVap.startAddr) / PG_SIZE;
        ASSERT(bitIdx >= 0)
        bitMapSet(&kernelVap.bitMap, bitIdx, 1);
    } else {
        putString("\n");
        putString("PoolFlag:");
        putUint32(pf);
        putString("\ncur->pageDir:");
        putUint32Hex((uint32) cur->pageDir);
        PANIC("PoolFlag and Vaddr not matched\n")
    }
    void* paddr = allocPhyAddr(pap);
    if (paddr == NULL) {
        return NULL;
    }
    mapPageTable((void*) vaddr, paddr);
    lockUnlock(&pap->lock);
    return (void*) vaddr;
}


void* sysMalloc(uint32 size) {
    PoolFlag pf;
    PhysicalAddrPool* pap;
    uint32 poolSize;
    MemBlockDesc* mbdArr;
    TaskStruct* cur = getCurrentThread();
    if (cur->pageDir == NULL) {
        pf = PF_KERNEL;
        poolSize = kernelPap.size;
        pap = &kernelPap;
        mbdArr = kmbdArr;
    } else {
        pf = PF_USER;
        poolSize = userPap.size;
        pap = &userPap;
        mbdArr = cur->umbdArr;
    }

    if (!(size > 0 && size < poolSize)) {
        return NULL;
    }

    Arena* arena;
    MemBlock* block;
    lockLock(&pap->lock);

    if (size > 1024) {
        uint32 pgCnt = DIV_ROUND_UP(size + sizeof(Arena), PG_SIZE);
        arena = allocPage(pf, pgCnt);
        if (arena != NULL) {
            arena->desc = NULL;
            arena->cnt = pgCnt;
            arena->large = true;
            lockUnlock(&pap->lock);
            return (void*) (arena + 1);
        } else {
            lockUnlock(&pap->lock);
            return NULL;
        }
    } else {
        uint8 descIdx;
        for (descIdx = 0; descIdx < MEM_BLOCK_DESC_CNT; descIdx++) {
            if (size <= mbdArr[descIdx].blockSize) {
                break;
            }
        }
        if (listIsEmpty(&mbdArr[descIdx].freeList)) {
            arena = allocPage(pf, 1);
            if (arena == NULL) {
                lockUnlock(&pap->lock);
                return NULL;
            }
            memset(arena, 0, PG_SIZE);
            arena->desc = &mbdArr[descIdx];
            arena->large = false;
            arena->cnt = mbdArr[descIdx].blocksPerArena;
            uint32 blockIdx = 0;

            IntrStatus intrStatus = getIntrStatus();
            disableIntr();
            for (blockIdx = 0; blockIdx < mbdArr[descIdx].blocksPerArena; blockIdx++) {
                block = arenaToBlock(arena, blockIdx);
                ASSERT(!listElemExist(&arena->desc->freeList, &block->freeElem))
                listAppend(&arena->desc->freeList, &block->freeElem);
            }
            setIntrStatus(intrStatus);
        }
        block = elemToEntry(MemBlock, freeElem, listPop(&mbdArr[descIdx].freeList));
        memset(block, 0, mbdArr[descIdx].blockSize);
        arena = blockToArena(block);
        arena->cnt--;
        lockUnlock(&pap->lock);
        return (void*) block;
    }
}

void sysFree(void* ptr) {
    ASSERT(ptr != NULL)
    if (ptr == NULL) {
        return;
    }
    PoolFlag pf;
    PhysicalAddrPool* pap;
    TaskStruct* cur = getCurrentThread();
    if (cur->pageDir == NULL) {
        pf = PF_KERNEL;
        pap = &kernelPap;
    } else {
        pf = PF_USER;
        pap = &userPap;
    }

    lockLock(&pap->lock);
    MemBlock* block = (MemBlock*) ptr;
    Arena* arena = blockToArena(block);
    ASSERT(arena->large == 0 || arena->large == 1)
    if (arena->large && arena->desc == NULL) {
        freePage(pf, arena, arena->cnt);
    } else {
        listAppend(&arena->desc->freeList, &block->freeElem);
        arena->cnt++;
        if (arena->cnt == arena->desc->blocksPerArena) {
            uint32 blockIdx;
            for (int blockIdx = 0; blockIdx < arena->desc->blocksPerArena; blockIdx++) {
                block = arenaToBlock(arena, blockIdx);
                ASSERT(listElemExist(&arena->desc->freeList,&block->freeElem))
                listRemove(&block->freeElem);
            }
            freePage(pf, arena, 1);
        }
    }
    lockUnlock(&pap->lock);
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

