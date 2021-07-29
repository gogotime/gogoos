bochs = bochs/bin
out_dir = out
kernel_dir= kernel
lib_dir = lib
disk_name = c.img


loader: boot.inc loader.asm mbr.asm
	nasm mbr.asm -I ./include -o ${out_dir}/mbr.bin
	nasm loader.asm -I ./include -o ${out_dir}/loader.bin

link:loader kernel lib
	make kernel
	make lib
	ld.lld -Ttext 0xc0001500 -e main -o ${out_dir}/kernel.bin ${out_dir}/kernel/main.o ${out_dir}/lib/kernel/print.o

copy_to_disk: link
	dd if=${out_dir}/mbr.bin of=${out_dir}/${disk_name} bs=512 count=1 conv=notrunc
	dd if=${out_dir}/loader.bin of=${out_dir}/${disk_name} seek=2 bs=512 count=4 conv=notrunc
	dd if=${out_dir}/kernel.bin of=${out_dir}/${disk_name} seek=9 bs=512 count=200 conv=notrunc



start:copy_to_disk
	${bochs}/bochs

