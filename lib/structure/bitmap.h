# ifndef __LIB_STRUCTURE_BITMAP_H
# define __LIB_STRUCTURE_BITMAP_H

#include "../stdint.h"

typedef struct {
    uint32 length;
    uint8* start;
} BitMap;

void bitMapInit(BitMap* bm);

int8 bitMapTest(BitMap* bm, uint32 bitIdx);

uint32 bitMapAlloc(BitMap* bm, uint32 cnt);

void bitMapSet(BitMap* bm, uint32 bitIdx, int8 val);

uint32 bitMapAllocAndSet(BitMap* bm, uint32 cnt, int8 val) ;

void bitMapPrint(BitMap* bm);

# endif