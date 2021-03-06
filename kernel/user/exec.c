#include "exec.h"
#include "../global.h"
#include "../memory.h"
#include "../../fs/fs.h"
#include "../../lib/string.h"
#include "../../lib/debug.h"
#include "../../lib/kernel/stdio.h"

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
} SegmentType;

static void printHex80(char* start) {
    uint8 cnt = 0;
    while (cnt < 80) {
        if (cnt % 10 == 0 && cnt != 0) {
            sysWrite(1, "\n", 1);
        }
        char buf[5] = {' '};
        memset(buf, ' ', 5);
        sprintk(buf, "%x ", start[cnt] & 0xff);
        sysWrite(1, buf, 5);
        cnt++;
    }
}


static bool segmentLoad(int32 fd, uint32 offset, uint32 fileSize, uint32 vaddr) {

    uint32 vaddrFirstPage = vaddr & 0xfffff000;
    uint32 sizeInFirstPage = PG_SIZE - (vaddr & 0x00000fff);
    uint32 occupyPages = 0;
    if (fileSize > sizeInFirstPage) {
        uint32 leftSize = fileSize - sizeInFirstPage;
        occupyPages = DIV_ROUND_UP(leftSize, PG_SIZE) + 1;
    } else {
        occupyPages = 1;
    }

    uint32 pageIdx = 0;
    uint32 vaddrPage = vaddrFirstPage;
    while (pageIdx < occupyPages) {
        uint32* pde = getPdePtr(vaddr);
        uint32* pte = getPtePtr(vaddr);
        if (!(*pde & 0x00000001) || !(*pte & 0x00000001)) {
            if (getOnePage(PF_USER, vaddrPage) == NULL) {
                return false;
            }
        }
        vaddrPage += PG_SIZE;
        pageIdx++;
    }
    sysLseek(fd, offset, SEEK_SET);
    sysRead(fd, (void*) vaddr, fileSize);
//    printHex80((char*) vaddr);
    return true;
}

static int32 load(const char* pathName) {
    int32 ret = -1;
    Elf32_Ehdr elfHeader;
    Elf32_Phdr programHeader;
    memset(&elfHeader, 0, sizeof(Elf32_Ehdr));
    int32 fd = sysOpen(pathName, O_RDONLY);
    if (fd == -1) {
        printk("load: open file %s failed\n", pathName);
        ret = -1;
        goto done;
    }
    if (sysRead(fd, &elfHeader, sizeof(Elf32_Ehdr)) != sizeof(Elf32_Ehdr)) {
        printk("load: read file %s failed\n", pathName);
        ret = -1;
        goto done;
    }
    if (memcmp(elfHeader.e_ident, "\x7f\x45\x4c\x46\x01\x01\x01", 7) ||
        elfHeader.e_type != 2 ||
        elfHeader.e_machine != 3 ||
        elfHeader.e_version != 1 ||
        elfHeader.e_phnum > 1024 ||
        elfHeader.e_phentsize != sizeof(Elf32_Phdr)) {
        ret = -1;
        goto done;
    }

    Elf32_Off programHeaderOffset = elfHeader.e_phoff;
    Elf32_Half programHeaderSize = elfHeader.e_phentsize;

    uint32 programIdx = 0;
    while (programIdx < elfHeader.e_phnum) {
//        printk("programIdx:%d\n", programIdx);
        memset(&programHeader, 0, programHeaderSize);
        sysLseek(fd, programHeaderOffset, SEEK_SET);
        if (sysRead(fd, &programHeader, programHeaderSize) != programHeaderSize) {
            printk("load: read file %s program header failed\n", pathName);
            ret = -1;
            goto done;
        }
        if (programHeader.p_type == PT_LOAD) {
            if (!segmentLoad(fd, programHeader.p_offset, programHeader.p_filesz, programHeader.p_vaddr)) {
                ret = -1;
                printk("load: segmentLoad failed\n");
                goto done;
            }
        }
        programHeaderOffset += elfHeader.e_phentsize;
        programIdx++;
    }
    ret = elfHeader.e_entry;
    done:
    sysClose(fd);
    return ret;
}

int32 sysExecv(const char* path, char* argv[16]) {
    uint32 argc = 0;
    while (argv[argc]) {
        argc++;
    }
    int32 entryPoint = load(path);
    if (entryPoint == -1) {
        return -1;
    }
//    printk("entry point:%x\n", entryPoint);

//    memcpy(cur->name, path, 16);
//    cur->name[15] = 0;

    TaskStruct* cur = getCurrentThread();
    IntrStack* intrStack = (IntrStack*) (((uint32) cur) + PG_SIZE - sizeof(IntrStack));
    intrStack->edi = intrStack->esi = intrStack->ebp = intrStack->espDummy = 0;
    intrStack->edx = intrStack->eax = 0;
    intrStack->ebx = argc;
    intrStack->ecx = (uint32) argv;
    intrStack->eip = (void*) entryPoint;
    intrStack->esp = (void*)0xc0000000;
    ASSERT(intrStack->esp != NULL)
//    printk("eip:%x \n", procStack->eip);
//    printk("esp:%x \n", procStack->esp);
    asm volatile ("movl %0,%%esp;"
                  "jmp intrExit;"
    :
    :"g"(intrStack)
    :"memory");
    return 0;
}
