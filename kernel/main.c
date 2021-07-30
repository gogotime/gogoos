
#include "../lib/kernel/print.h"


int _start(){

    __asm{
    mov eax,1000
    mov ebx,2000
    mov ecx,3000
    mov edx,4000
    }
    putString("hello world\n");
//    putUint32(4294967295);

    putString("hello world\n");
    int32 x=100000;
    while (x--){
        putUint32(x);
        putChar('\n');
    }
//    putChar('\b');
//    putString("\n");
//    putUint32Hex(4294967295);
    __asm{
    mov eax,1000
    mov ebx,123124
    mov ecx,436236
    mov edx,123124
    }
    while(1);


    return 0;
}