out_dir = ../out/kernel

all:
	clang -m32 -O3 -S -nostdlib -ffreestanding -fasm-blocks -masm=intel main.c -o ${out_dir}/main.s
	clang  -m32 -O3 -c  ${out_dir}/main.s -o ${out_dir}/main.o

