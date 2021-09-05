# include "asm/print.h"
# include "../stdio.h"
# include <stdarg.h>

void putString(char* string) {
    while (*string) {
        sysPutChar(*string);
        string++;
    }
}

void putUint32(uint32 val) {
    if (val == 0) {
        sysPutChar('0');
        return;
    }
    int8 idx = 0;
    char remain;
    char buff[10] = {0};
    while (val) {
        remain = val % 10;
        val = val / 10;
        buff[idx] = remain + '0';
        idx++;
    }
    idx--;
    while (idx >= 0) {
        sysPutChar(buff[idx]);
        idx--;
    }
}

void putUint32Hex(uint32 val) {
    if (val == 0) {
        sysPutChar('0');
        sysPutChar('x');
        sysPutChar('0');
        return;
    }
    int8 idx = 0;
    uint8 remain;
    char buff[8] = {0};
    while (val) {
        remain = val % 16;
        val = val / 16;
        buff[idx] = remain + '0';
        idx++;
    }
    idx--;
    sysPutChar('0');
    sysPutChar('x');
    while (idx >= 0) {
        if (buff[idx] > '9') {
            buff[idx] += 7;
        }
        sysPutChar(buff[idx]);
        idx--;
    }
}
