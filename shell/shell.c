#include "../lib/user/syscall.h"
#include "../lib/stdio.h"
#include "../lib/debug.h"
#include "../lib/string.h"
#include "../fs/file.h"
#include "../fs/fs.h"


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

static int32 cmdParse(char* cmdStr, char** argv, char token) {
    ASSERT(cmdStr != NULL)
    int32 argIdx = 0;
    while (argIdx < MAX_ARG_NUM) {
        argv[argIdx] = NULL;
        argIdx++;
    }
    char* next = cmdStr;
    int32 argc = 0;
    while (*next) {
        while (*next == token) {
            next++;
        }
        if (*next == 0) {
            break;
        }
        argv[argc] = next;
        while (*next && *next != token) {
            next++;
        }
        if (*next) {
            *next++ = 0;
        }
        if (argc > MAX_ARG_NUM) {
            return -1;
        }
        argc++;
    }
    return argc;
}

char* argv[MAX_ARG_NUM];
int32 argc = -1;

void myShell() {
    cwdCache[0] = '/';
    cwdCache[1]=0;
    while (1) {
        printPrompt();
//        memset(finalPath, 0, MAX_PATH_LEN);
        memset(cmdLine, 0, CMD_LEN);
        readLine(cmdLine, CMD_LEN);
        if (cmdLine[0] == 0) {
            continue;
        }
        argc = -1;
        argc = cmdParse(cmdLine, argv, ' ');
        if (argc == -1) {
            printf(" num of arg exceeded!\n");
            continue;
        }
        int32 argIdx = 0;
        while (argIdx < argc) {
            printf("%s ", argv[argIdx]);
            argIdx++;
        }
        printf("\n");
    }
    PANIC("myShell:should not be here\n")
}