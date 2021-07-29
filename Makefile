BOCHS = bochs/bin
OUT_DIR = out
DISK_NAME = c.img
CC=clang
CFLAGS= -c -m32 -nostdlib -ffreestanding
ASM=nasm
ASMFLAGS= -f elf32

CMP_DIRS=lib kernel
SCH_DIRS = $(shell find $(CMP_DIRS) -maxdepth 9 -type d)
OUT_CRT_DIRS+=$(foreach dir, $(SCH_DIRS), $(OUT_DIR)/$(dir) )
SRCS_C += $(foreach dir, $(SCH_DIRS), $(wildcard $(dir)/*.c))
SRCS_ASM += $(foreach dir, $(SCH_DIRS), $(wildcard $(dir)/*.asm))
OBJS_C +=$(patsubst %.c, %.o, $(SRCS_C))
OBJS_ASM +=$(patsubst %.asm, %.o, $(SRCS_ASM))
OBJS_OUT_ALL+=$(foreach dir, $(OBJS_C), $(OUT_DIR)/$(dir) )
OBJS_OUT_ALL+=$(foreach dir, $(OBJS_ASM), $(OUT_DIR)/$(dir) )
all:$(OBJS_C)  $(OBJS_ASM)

loader: include/boot.inc loader.asm mbr.asm
	nasm mbr.asm -I ./include -o ${OUT_DIR}/mbr.bin
	nasm loader.asm -I ./include -o ${OUT_DIR}/loader.bin

make_out_dir:
	mkdir -p ${OUT_DIR}
	for x in $(OUT_CRT_DIRS); do \
    	 mkdir -p $$x; \
    done

$(OBJS_C):%.o : %.c make_out_dir
	$(CC) $(CFLAGS) $< -o $(OUT_DIR)/$@

$(OBJS_ASM):%.o : %.asm make_out_dir
	$(ASM) $(ASMFLAGS) $< -o $(OUT_DIR)/$@

link:$(OBJS_C) $(OBJS_ASM)
	ld.lld -Ttext 0xc0001500 -e main -o $(OUT_DIR)/kernel.bin  ${OBJS_OUT_ALL}

copy_to_disk: link loader
	dd if=${OUT_DIR}/mbr.bin of=${OUT_DIR}/${DISK_NAME} bs=512 count=1 conv=notrunc
	dd if=${OUT_DIR}/loader.bin of=${OUT_DIR}/${DISK_NAME} seek=2 bs=512 count=4 conv=notrunc
	dd if=${OUT_DIR}/kernel.bin of=${OUT_DIR}/${DISK_NAME} seek=9 bs=512 count=200 conv=notrunc


start:copy_to_disk
	${BOCHS}/bochs

