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


// ------------- GDT -------------

#define DESC_G_4K     1
#define DESC_D_32     1
#define DESC_L        0
#define DESC_AVL      0
#define DESC_P        1
#define DESC_DPL_0    0
#define DESC_DPL_1    1
#define DESC_DPL_2    2
#define DESC_DPL_3    3

#define DESC_S_DATA   1
#define DESC_S_CODE   1
#define DESC_S_SYS    0

#define DESC_TYPE_CODE 8
#define DESC_TYPE_DATA 2
#define DESC_TYPE_TSS  9

#define GDT_ADDR_HIGH ((DESC_G_4K<<7)+(DESC_D_32<<6)+(DESC_L<<5)+(DESC_AVL<<4))
#define GDT_ADDR_LOW_CODE_DPL3 ((DESC_P<<7)+(DESC_DPL_3<<5)+(DESC_S_CODE<<4)+DESC_TYPE_CODE)
#define GDT_ADDR_LOW_DATA_DPL3 ((DESC_P<<7)+(DESC_DPL_3<<5)+(DESC_S_DATA<<4)+DESC_TYPE_DATA)

// ------------- TSS -------------

#define TSS_DESC_D 0

#define TSS_ADDR_HIGH ((DESC_G_4K<<7)+(TSS_DESC_D<<6)+(DESC_L<<5)+(DESC_AVL<<4))
#define TSS_ADDR_LOW ((DESC_P<<7)+(DESC_DPL_0<<5)+(DESC_S_SYS<<4)+DESC_TYPE_TSS)

// ---------- SELECTOR ----------

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
#define SELECTOR_TSS     ((4<<3) + (TI_GDT <<2) + RPL0)
#define SELECTOR_U_CODE  ((5<<3) + (TI_GDT <<2) + RPL3)
#define SELECTOR_U_DATA  ((6<<3) + (TI_GDT <<2) + RPL3)
#define SELECTOR_U_STACK SELECTOR_U_DATA

// ------------- IDT -------------

#define IDT_DESC_P 1     //present bit
#define IDT_DESC_DPL0 0
#define IDT_DESC_DPL3 3
#define IDT_DESC_32_TYPE 0xE

#define IDT_DESC_ATTR_DPL0 ((IDT_DESC_P<<7) + (IDT_DESC_DPL0 <<5) + IDT_DESC_32_TYPE)
#define IDT_DESC_ATTR_DPL3 ((IDT_DESC_P<<7) + (IDT_DESC_DPL3 <<5) + IDT_DESC_32_TYPE)
#endif