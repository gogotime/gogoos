#include "../string.h"
#include "bitmap.h"
#include "../kernel/asm/put_char.h"

void bitMapInit(BitMap* bm) {
    memset(bm->startAddr, 0, bm->length);
}

int8 bitMapTest(BitMap* bm, uint32 bitIdx) {
    if (bitIdx >= bm->length * 8) {
        return 0;
    }
    uint32 byteIdx = bitIdx / 8;
    bitIdx = bitIdx % 8;
    return ((bm->startAddr[byteIdx]) >> bitIdx) & 0x1;
}

uint32 bitMapScan(BitMap* bm, uint32 cnt){
    uint32 byteIdx = 0;
    while ((bm->startAddr[byteIdx] == 0xff) && (byteIdx < bm->length)) {
        byteIdx++;
    }
    if (byteIdx == bm->length) {
        return -1;
    }

    uint32 bitIdx = 0;
    while (((bm->startAddr[byteIdx]) >> bitIdx) & 0x1) {
        bitIdx++;
    }
    uint32 bitIdxStart = 8 * byteIdx + bitIdx;
    if (cnt == 1) {
        return bitIdxStart;
    }
    uint32 leftBit = bm->length * 8 - bitIdxStart - 1;
    uint32 nextBit = bitIdxStart + 1;
    uint32 fdCnt = 1;
    bitIdxStart = -1;
    while (leftBit-- > 0) {
        if (!bitMapTest(bm, nextBit)) {
            fdCnt++;
        } else {
            fdCnt = 0;
        }
        if (fdCnt == cnt) {
            bitIdxStart = nextBit - cnt + 1;
            break;
        }
        nextBit++;
    }
    return bitIdxStart;
}

void bitMapSet(BitMap* bm, uint32 bitIdx, int8 val) {
    uint32 byteIdx = bitIdx / 8;
    bitIdx = bitIdx % 8;
    if (val) {
        bm->startAddr[byteIdx] |= (uint8) (0x1 << bitIdx);
    } else {
        bm->startAddr[byteIdx] &= (uint8) ~(0x1 << bitIdx);
    }
}

uint32 bitMapScanAndSet(BitMap* bm, uint32 cnt, int8 val) {
    uint32 bitIdxStart = bitMapScan(bm, cnt);
    if (bitIdxStart == -1) {
        return -1;
    }
    uint32 idx = bitIdxStart;
    while (cnt--) {
        bitMapSet(bm, idx, val);
        idx++;
    }
    return bitIdxStart;
}

void bitMapPrint(BitMap* bm) {
    uint32 byteIdx = 0;
    while (byteIdx < bm->length) {
        uint8 bitIdx = 0;
        while (bitIdx < 8) {
            putChar('0' + ((bm->startAddr[byteIdx] >> bitIdx) & 0x1));
            bitIdx++;
        }
        putChar(' ');
        byteIdx++;
    }
    putChar('\n');
}
