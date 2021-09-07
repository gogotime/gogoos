#include "exec.h"
#include "../global.h"

extern uint32 intrExit;

typedef uint32 Elf32_Word, Elf32_Off, Elf32_Addr;
typedef uint16 Elf32_Half;
#define EI_NIDENT 16
typedef struct {
    unsigned char e_ident[EI_NIDENT];     /* Magic number和其它信息 */
    Elf32_Half e_type;                    /* Object file type */
    Elf32_Half e_machine;                 /* Architecture */
    Elf32_Word e_version;                 /* Object file version */
    Elf32_Addr e_entry;                   /* Entry point virtual address */
    Elf32_Off e_phoff;                    /* Program header table file offset */
    Elf32_Off e_shoff;                    /* Section header table file offset */
    Elf32_Word e_flags;                   /* Processor-specific flags */
    Elf32_Half e_ehsize;                  /* ELF header size in bytes */
    Elf32_Half e_phentsize;               /* Program header table entry size */
    Elf32_Half e_phnum;                   /* Program header table entry count */
    Elf32_Half e_shentsize;               /* Section header table entry size */
    Elf32_Half e_shnum;                   /* Section header table entry count */
    Elf32_Half e_shstrndx;                /* Section header string table index */
} Elf32_Ehdr;

typedef struct {
    Elf32_Word p_type;          /* Segment type */
    Elf32_Off p_offset;         /* Segment file offset */
    Elf32_Addr p_vaddr;         /* Segment virtual address */
    Elf32_Addr p_paddr;         /* Segment physical address */
    Elf32_Word p_filesz;        /* Segment size in file */
    Elf32_Word p_memsz;         /* Segment size in memory */
    Elf32_Word p_flags;         /* Segment flags */
    Elf32_Word p_align;         /* Segment alignment */
} Elf32_Phdr;

typedef enum {
    PT_NULL,
    PT_LOAD,
    PT_DYNAMIC,
    PT_INTERP,
    PT_NOTE,
    PT_SHLIB,
    PT_PHDR,
}SegmentType;

static bool segmentLoad(int32 fd,uint32 offset,uint32 fileSIze,uint32 vaddr){
    uint32 vaddrFirstPage = vaddr & 0xfffff000;
    uint32 sizeInFirstPage = PG_SIZE - (vaddr & 0x00000fff);
    uint32 occupyPages = 0;

}