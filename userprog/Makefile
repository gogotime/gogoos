
echo:


test1: echo
	clang -m32 -ffunction-sections -fdata-sections -fno-builtin -ffreestanding -o test1.o -c test1.c
	ld -m elf_i386 -Ttext 0x10000 --gc-sections -e main test1.o ../out/lib/string.o ../out/lib/user/stdio.o ../out/lib/user/syscall.o -o test1
	dd if=test1 of=../c.img bs=512 count=30 seek=400 conv=notrunc