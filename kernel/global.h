#ifndef __LIB_GLOBAL_H
#define __LIB_GLOBAL_H

#include "../lib/stdint.h"

#define PG_SIZE 4096
#define MEM_BITMAP_BASE 0xc009a000

#define K_HEAP_START 0xc0100000
#define PAGE_DIR_TABLE_POS 0x00100000
#define LOADER_BASE_ADDR 0x900
#define MEMORY_DATA_ADDR 0x904

#define PDE_IDX(addr) ((addr & 0xffc00000)>>22)
#define PTE_IDX(addr) ((addr & 0x003ff000)>>12)


#define RPL0 0
#define RPL1 1
#define RPL2 2
#define RPL3 3

#define TI_GDT 0
#define TI_LDT 1

#define SELECTOR_K_CODE  ((1<<3) + (TI_GDT <<2) + RPL0)
#define SELECTOR_K_DATA  ((2<<3) + (TI_GDT <<2) + RPL0)
#define SELECTOR_K_STACK SELECTOR_K_DATA
#define SELECTOR_K_VIDEO ((3<<3) + (TI_GDT <<2) + RPL0)

#define IDT_DESC_P 1     //present bit
#define IDT_DESC_DPL0 0
#define IDT_DESC_DPL3 3
#define IDT_DESC_32_TYPE 0xE

#define IDT_DESC_ATTR_DPL0 ((IDT_DESC_P<<7) + (IDT_DESC_DPL0 <<5) + IDT_DESC_32_TYPE)
#define IDT_DESC_ATTR_DPL3 ((IDT_DESC_P<<7) + (IDT_DESC_DPL3 <<5) + IDT_DESC_32_TYPE)
#endif