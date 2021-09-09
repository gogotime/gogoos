#include "../lib/user/syscall.h"
#include "../lib/user/stdio.h"
#include "../lib/debug.h"
#include "../lib/string.h"
#include "../fs/file.h"
#include "../fs/fs.h"
#include "buildin_cmd.h"

#define CMD_LEN 128
#define MAX_ARG_NUM 16

static char cmdLine[CMD_LEN] = {0};

char cwdCache[MAX_PATH_LEN] = {0};

void printPrompt() {
    getCwd(cwdCache, MAX_PATH_LEN);
    printf("[galamo@localhost %s]$ ", cwdCache);
}

static void readLine(char* buf, int32 count) {
    if (!(buf != NULL && count > 0)) {
        printf("failed to readline\n");
        return;
    }
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
    if (cmdStr == NULL) {
        printf("failed to parse\n");
        return 0;
    }
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
        char buf[MAX_PATH_LEN] = {0};

        if (!strcmp("pwd", argv[0])) {
            buildinPwd(argc, argv);
        } else if (!strcmp("cd", argv[0])) {
            buildinCd(argc, argv);
        } else if (!strcmp("ls", argv[0])) {
            buildinLs(argc, argv);
        } else if (!strcmp("ps", argv[0])) {
            buildinPs(argc, argv);
        } else if (!strcmp("clear", argv[0])) {
            buildinClear(argc, argv);
        }else if (!strcmp("mkdir", argv[0])) {
            buildinMkdir(argc, argv);
        }else if (!strcmp("rmdir", argv[0])) {
            buildinRmdir(argc, argv);
        }else if (!strcmp("touch", argv[0])) {
            buildinTouch(argc, argv);
        }else if (!strcmp("rm", argv[0])) {
            buildinRm(argc, argv);
        }else if (!strcmp("wf", argv[0])) {
            buildinWf(argc, argv);
        }else if (!strcmp("rf", argv[0])) {
            buildinRf(argc, argv);
        }else {
            char finalPath[MAX_PATH_LEN];
            makeClearAbsPath(argv[0], finalPath);
//            argv[0] = finalPath;
            Stat fileStat;
            memset(&fileStat, 0, sizeof(Stat));
            if (stat(finalPath, &fileStat) == -1) {
                printf("shell: no such file %s\n", finalPath);
            }else{
                int32 pid = fork();
                if (pid) {
                    int32 status;
                    wait(&status);
                } else {
                    execv(finalPath, argv);
                }
            }
            int32 argIdx = 0;
            while (argIdx < argc) {
                argv[argc] = NULL;
                argIdx++;
            }
            printf("\n");
        }


    }
    PANIC("myShell:should not be here\n")
}