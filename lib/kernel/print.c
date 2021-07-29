# include "asm/put_char.h"

// void put_char(uint8 char_ascii){
//    put_char0(char_ascii);
//}
void put_char(char char_ascii){
    put_char_asm(char_ascii);
}

void put_string(char *string){
    while (*string){
        put_char(*string);
        string++;
    }
}