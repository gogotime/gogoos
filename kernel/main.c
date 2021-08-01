
#include "../lib/kernel/print.h"
#include "../lib/kernel/io.h"

int _start() {

    __asm{
    mov eax, 1000
    mov ebx, 2000
    mov ecx, 3000
    mov edx, 4000
    }

    putString("hello world\n");
    outb(1234, 232);
    int p=inb(1234);
    putUint32(p);
    putUint32Hex(0xffffffff);
    __asm{
    mov eax, 1000
    mov ebx, 123124
    mov ecx, 436236
    mov edx, 123124
    }
    while (1);


    return 0;
}