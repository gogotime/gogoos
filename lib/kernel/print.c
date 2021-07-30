# include "asm/put_char.h"

// void put_char(uint8 char_ascii){
//    put_char0(char_ascii);
//}

void putString(char *string){
    while (*string){
        putChar(*string);
        string++;
    }
}

void putUint32(uint32 val){
    if (val==0){
        putChar('0');
        return;
    }
    int8 idx=0;
    char remain;
    char buff[10]={0};
    while (val){
        remain=val % 10;
        val=val/10;
        buff[idx]=remain+'0';
        idx++;
    }
    idx--;
    while (idx>=0){
        putChar(buff[idx]);
        idx--;
    }
}

void putUint32Hex(uint32 val){
    if (val==0){
        putChar('0');
        putChar('x');
        putChar('0');
        return;
    }
    int8 idx=0;
    char remain;
    char buff[8]={0};
    while (val){
        remain=val % 16;
        val=val/16;
        if (remain<10) {
            buff[idx]=remain+'0';
        }else{
            buff[idx]=remain+'a'-10;
        }
        idx++;
    }
    idx--;
    while (idx>=0){
        putChar(buff[idx]);
        idx--;
    }
}