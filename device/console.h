#ifndef DEVICE_CONSOLE_H
#define DEVICE_CONSOLE_H

void consoleInit();

void consolePutString(char* string);

void consolePutUint32(uint32 val);

void consolePutUint32Hex(uint32 val);

void consolePutChar(char c);

#endif