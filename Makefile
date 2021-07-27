bochs = bochs/bin
out_dir = out
kernel_dir= kernel

knl:
	gcc -m32 -O3 -ffunction-sections -fdata-sections -c  ${kernel_dir}/main.c -o ${kernel_dir}/main.o
	ld -m elf_i386 -O -gc-sections ${kernel_dir}/main.o -Ttext 0xc0001500 -e main -o ${kernel_dir}/kernel.bin
	dd if=${kernel_dir}/kernel.bin of=${out_dir}/c.img seek=9 bs=512 count=200 conv=notrunc

asm:
	nasm mbr.asm -I ./include -o ${out_dir}/mbr.bin
	nasm loader.asm -I ./include -o ${out_dir}/loader.bin
	dd if=${out_dir}/mbr.bin of=${out_dir}/c.img bs=512 count=1 conv=notrunc
	dd if=${out_dir}/loader.bin of=${out_dir}/c.img seek=2 bs=512 count=4 conv=notrunc


start:asm knl
	${bochs}/bochs
