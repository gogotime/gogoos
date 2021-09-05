#include "../lib/user/syscall.h"
#include "../lib/stdio.h"
#include "../lib/debug.h"
#include "../lib/string.h"
#include "../fs/file.h"


#define CMD_LEN 128
#define MAX_ARG_NUM 16

static char cmdLine[CMD_LEN] = {0};

char cwdCache[64] = {0};

void printPrompt() {
    printf("[galamo@localhost %s]$ ", cwdCache);
}

static void readLine(char* buf, int32 count) {
    ASSERT(buf != NULL && count > 0)
    char* pos = buf;
    while (read(stdin, pos, 1) != -1 && (pos - buf) < count) {
        switch (*pos) {
            case '\n':
            case '\r':
                *pos = 0;
                putChar('\n');
                return;
            case '\b':
                if (buf[0] != '\b') {
                    --pos;
                    putChar('\b');
                }
                break;
            case 'l' - 'a':
                *pos = 0;
                clear();
                printPrompt();
                printf("%s", buf);
                break;
            case 'u' - 'a':
                while (buf != pos) {
                    putChar('\b');
                    *(pos) = 0;
                    pos--;
                }
                break;
            default:
                putChar(*pos);
                pos++;
        }
    }
    printf("readline:shell input out of length");
}

void myShell(){
    cwdCache[0] = '/';
    while (1) {
        printPrompt();
        memset(cmdLine, 0, CMD_LEN);
        readLine(cmdLine, CMD_LEN);
        if (cmdLine[0] == 0) {
            continue;
        }
    }
    PANIC("myShell:should not be here\n")
}