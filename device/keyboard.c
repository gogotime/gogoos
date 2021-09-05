#include "keyboard.h"
#include "../lib/kernel/print.h"
#include "../lib/kernel/io.h"
#include "../lib/stdint.h"
#include "../lib/structure/ioqueue.h"
#include "../kernel/interrupt.h"

#define KBD_BUF_PORT 0x60

#define ESC '\x1b'
#define BACKSPACE '\b'
#define TAB '\t'
#define ENTER '\r'
#define DELETE '\x7f'

#define CHAR_INVISIBLE 0
#define CTRL_L_CHAR CHAR_INVISIBLE
#define CTRL_R_CHAR CHAR_INVISIBLE
#define SHIFT_L_CHAR CHAR_INVISIBLE
#define SHIFT_R_CHAR CHAR_INVISIBLE
#define ALT_L_CHAR CHAR_INVISIBLE
#define ALT_R_CHAR CHAR_INVISIBLE
#define CAPS_LOCK_CHAR CHAR_INVISIBLE

#define CTRL_L_MAKE 0x1d
#define CTRL_R_MAKE 0xe01d
#define SHIFT_L_MAKE 0x2a
#define SHIFT_R_MAKE 0x36
#define ALT_L_MAKE 0x38
#define ALT_R_MAKE 0xe038
#define CAPS_LOCK_MAKE 0x3a

#define CTRL_L_BREAK 0x9d
#define CTRL_R_BREAK 0xe09d
#define SHIFT_L_BREAK 0xaa
#define SHIFT_R_BREAK 0xb6
#define ALT_L_BREAK 0xb8
#define ALT_R_BREAK 0xe0b8
#define CAPS_LOCK_BREAK 0xba

static char keymap[][2] = {
/* 0x00 */
        {0,    0},
/* 0x01 */
        {ESC,            ESC},
/* 0x02 */
        {'1',  '!'},
/* 0x03 */
        {'2',  '@'},
/* 0x04 */
        {'3',  '#'},
/* 0x05 */
        {'4',  '$'},
/* 0x06 */
        {'5',  '%'},
/* 0x07 */
        {'6',  '^'},
/* 0x08 */
        {'7',  '&'},
/* 0x09 */
        {'8',  '*'},
/* 0x0A */
        {'9',  '('},
/* 0x0B */
        {'0',  ')'},
/* 0x0C */
        {'-',  '_'},
/* 0x0D */
        {'=',  '+'},
/* 0x0E */
        {BACKSPACE,      BACKSPACE},
/* 0x0F */
        {TAB,            TAB},
/* 0x10 */
        {'q',  'Q'},
/* 0x11 */
        {'w',  'W'},
/* 0x12 */
        {'e',  'E'},
/* 0x13 */
        {'r',  'R'},
/* 0x14 */
        {'t',  'T'},
/* 0x15 */
        {'y',  'Y'},
/* 0x16 */
        {'u',  'U'},
/* 0x17 */
        {'i',  'I'},
/* 0x18 */
        {'o',  'O'},
/* 0x19 */
        {'p',  'P'},
/* 0x1A */
        {'[',  '{'},
/* 0x1B */
        {']',  '}'},
/* 0x1C */
        {ENTER,          ENTER},
/* 0x1D */
        {CTRL_L_CHAR,    CTRL_L_CHAR},
/* 0x1E */
        {'a',  'A'},
/* 0x1F */
        {'s',  'S'},
/* 0x20 */
        {'d',  'D'},
/* 0x21 */
        {'f',  'F'},
/* 0x22 */
        {'g',  'G'},
/* 0x23 */
        {'h',  'H'},
/* 0x24 */
        {'j',  'J'},
/* 0x25 */
        {'k',  'K'},
/* 0x26 */
        {'l',  'L'},
/* 0x27 */
        {';',  ':'},
/* 0x28 */
        {'\'', '"'},
/* 0x29 */
        {'`',  '~'},
/* 0x2A */
        {SHIFT_L_CHAR,   SHIFT_L_CHAR},
/* 0x2B */
        {'\\', '|'},
/* 0x2C */
        {'z',  'Z'},
/* 0x2D */
        {'x',  'X'},
/* 0x2E */
        {'c',  'C'},
/* 0x2F */
        {'v',  'V'},
/* 0x30 */
        {'b',  'B'},
/* 0x31 */
        {'n',  'N'},
/* 0x32 */
        {'m',  'M'},
/* 0x33 */
        {',',  '<'},
/* 0x34 */
        {'.',  '>'},
/* 0x35 */
        {'/',  '?'},
/* 0x36 */
        {SHIFT_R_CHAR,   SHIFT_R_CHAR},
/* 0x37 */
        {'*',  '*'},
/* 0x38 */
        {ALT_R_CHAR,     ALT_R_CHAR},
/* 0x39 */
        {' ',  ' '},
/* 0x3A */
        {CAPS_LOCK_CHAR, CAPS_LOCK_CHAR},
};

static bool ctrlStatus, shiftStatus, altStatus, capsLockStatus, extScanCode;
IOQueue keyboardBuf;

static void intrKeyBoardHandler(uint8 intrNr) {
    uint16 scanCode = inb(KBD_BUF_PORT);
    if (scanCode == 0xe0) {
        extScanCode = true;
        return;
    }
    if (extScanCode) {
        scanCode = (0xe800 | scanCode);
        extScanCode = false;
    }
    bool isBreakCode = (scanCode & 0x0080);
    if (isBreakCode) {
        if (scanCode == CTRL_L_BREAK || scanCode == CTRL_R_BREAK) {
            ctrlStatus = false;
        } else if (scanCode == SHIFT_L_BREAK || scanCode == SHIFT_R_BREAK) {
            shiftStatus = false;
        } else if (scanCode == ALT_L_BREAK || scanCode == ALT_R_BREAK) {
            altStatus = false;
        }
        return;
    } else if ((scanCode > 0x0 && scanCode <= 0x3a) || scanCode == CTRL_R_MAKE || scanCode == ALT_R_MAKE) {
        switch (scanCode) {
            case SHIFT_L_MAKE:
            case SHIFT_R_MAKE:
                shiftStatus = true;
                break;
            case CTRL_L_MAKE:
            case CTRL_R_MAKE:
                ctrlStatus = true;
                break;
            case ALT_L_MAKE:
            case ALT_R_MAKE:
                altStatus = true;
                break;
            case CAPS_LOCK_MAKE:
                if (!capsLockStatus) {
                    capsLockStatus = true;
                }
                break;
            case CAPS_LOCK_BREAK:
                if (capsLockStatus) {
                    capsLockStatus = false;
                }
                break;
            default: {
                bool shift = false;
                if (shiftStatus) {
                    shift = !shift;
                }
                if (capsLockStatus) {
                    if (!(scanCode < 0x0e || scanCode == 0x29 || scanCode == 0x1a || scanCode == 0x1b ||
                          scanCode == 0x2b || scanCode == 0x27 || scanCode == 0x28 || scanCode == 0x33 ||
                          scanCode == 0x34 || scanCode == 0x35)) {
                        shift = !shift;
                    }
                }
                uint8 idx = (scanCode & 0x00ff);
                char c = keymap[idx][shift];
                if (ctrlStatus) {
                    if (c == 'l' || c == 'L') {
                        c = 'l' - 'a';
                    }
                    if (c == 'u' || c == 'U') {
                        c = 'u' - 'a';
                    }
                }
                if (c) {
                    ioQueuePutChar(&keyboardBuf, c);
                }
            }
        }
    }
    return;
}

void keyBoardInit() {
    putString("keyBoardInit start\n");
    ioQueueInit(&keyboardBuf);
    registerIntrHandler(0x21, intrKeyBoardHandler);
    putString("keyBoardInit done\n");
}

