#include "../lib/user/syscall.h"
#include "../lib/user/stdio.h"
#include "../lib/debug.h"
#include "../lib/string.h"
#include "../fs/file.h"
#include "../fs/fs.h"


static void washPath(char* oldAbsPath, char* newAbsPath) {
    if (oldAbsPath[0] != '/') {
        printf("failed to washPath\n");
        return;
    }
    char name[MAX_PATH_LEN] = {0};
    char* subPath = oldAbsPath;
    subPath = pathParse(subPath, name);
    if (name[0] == 0) {
        newAbsPath[0] = '/';
        newAbsPath[1] = 0;
        return;
    }
    newAbsPath[0] = 0;
    strcat(newAbsPath, "/");
    while (name[0]) {
        if (!strcmp("..", name)) {
            char* slashPtr = strrchr(newAbsPath, '/');
            if (slashPtr != newAbsPath) {
                *slashPtr = 0;
            } else {
                *(slashPtr + 1) = 0;
            }
        } else if (strcmp(".", name)) {
            if (strcmp(newAbsPath, "/")) {
                strcat(newAbsPath, "/");
            }
            strcat(newAbsPath, name);
        }
        memset(name, 0, MAX_FILE_NAME_LEN);
        if (subPath) {
            subPath = pathParse(subPath, name);
        }
    }
}

void makeClearAbsPath(char* path, char* finalPath) {
    char absPath[MAX_PATH_LEN] = {0};
    if (path[0] != '/') {
        memset(absPath, 0, MAX_PATH_LEN);
        if (getCwd(absPath, MAX_PATH_LEN) != NULL) {
            if (!(absPath[0] == '/' && absPath[1] == 0)) {
                strcat(absPath, "/");
            }
        }
    }
    strcat(absPath, path);
    washPath(absPath, finalPath);
}

void buildinPwd(uint32 argc, char** argv) {
    if (argc != 1) {
        printf("pwd: no argument support\n");
        return;
    }
    char finalPath[MAX_PATH_LEN] = {0};
    if (getCwd(finalPath, MAX_PATH_LEN) != NULL) {
        printf("%s\n", finalPath);
    } else {
        printf("pwd: get current work directory failed\n");
    }
}

int32 buildinCd(uint32 argc, char** argv) {
    if (argc > 2) {
        printf("cd: only support 1 argument!\n");
        return -1;
    }
    char finalPath[MAX_PATH_LEN] = {0};
    if (argc == 1) {
        finalPath[0] = '/';
        finalPath[1] = 0;
    } else {
        makeClearAbsPath(argv[1], finalPath);
    }
    if (chdir(finalPath) == -1) {
        printf("cd: no such directory %s\n", finalPath);
        return -1;
    }
    return 0;
}

void buildinLs(uint32 argc, char** argv) {
    if (argc > 2) {
        printf("ls: only support 1 argument!\n");
        return;
    }
    Dir* dir = NULL;
    char dirPath[MAX_PATH_LEN] = {0};
    if (argc == 2) {
        makeClearAbsPath(argv[1], dirPath);
        dir = openDir(dirPath);
    } else {
        getCwd(dirPath, MAX_PATH_LEN);
        dir = openDir(dirPath);
    }

    if (dir == NULL) {
        printf("ls: %s is not a directory or not existed\n",dirPath);
        return;
    }
    if (!strcmp(dirPath, "/")) {
        rewindDir(dir);
    }

    char* title = "INO        TYPE        NAME            SIZE\n";
    write(1, title, strlen(title));
    DirEntry* de = NULL;
    while ((de = readDir(dir)) != NULL) {
        char buf[16] = {' '};
        memset(buf, ' ', 11);
        sprintf(buf, "%d", de->ino);
        write(1, buf, 11);

        switch (de->fileType) {
            case FT_UNKNOWN:
                write(1, "UNKNOWN     ", 13);
                break;
            case FT_DIRECTORY:
                write(1, "DIRECTORY   ", 13);
                break;
            case FT_REGULAR:
                write(1, "FILE        ", 13);
                break;
        }

        memset(buf, ' ', 16);
        sprintf(buf, "%s", de->fileName);
        for (int32 i = 0; i < 16; i++) {
            if (buf[i] == 0) {
                buf[i] = ' ';
            }
        }
        write(1, buf, 16);

        memset(buf, ' ', 16);
        Stat s;
        char filePath[MAX_PATH_LEN] = {0};
        strcat(filePath, dirPath);
        strcat(filePath, "/");
        strcat(filePath, de->fileName);
        stat(filePath,&s);
        sprintf(buf, "%d", s.size);
        write(1, buf, 11);

        write(1, "\n", 1);
    }
    closeDir(dir);
}

void buildinPs(uint32 argc, char** argv) {
    if (argc != 1) {
        printf("ps: no argument support\n");
        return;
    }
    ps();
}

void buildinClear(uint32 argc, char** argv) {
    if (argc != 1) {
        printf("clear: no argument support\n");
        return;
    }
    clear();
}

void buildinMkdir(uint32 argc, char** argv) {
    if (argc != 2) {
        printf("mkdir: only support 1 argument\n");
        return;
    }
    char finalPath[MAX_PATH_LEN] = {0};
    makeClearAbsPath(argv[1], finalPath);
    if (strcmp("/", finalPath)) {
        if (mkdir(finalPath) == -1) {
            printf("mkdir: create directory %s failed\n", finalPath);
        }
    }
}

void buildinRmdir(uint32 argc, char** argv) {
    if (argc != 2) {
        printf("rmdir: only support 1 argument\n");
        return;
    }
    char finalPath[MAX_PATH_LEN] = {0};
    makeClearAbsPath(argv[1], finalPath);
    if (strcmp("/", finalPath)) {
        char buf[MAX_PATH_LEN];
        getCwd(buf, MAX_PATH_LEN);
        if (!strcmp(buf, finalPath)) {
            printf("rmdir: can't delete current working directory\n");
            return;
        }
        if (rmdir(finalPath) == -1) {
            printf("rmdir: remove directory %s failed\n", finalPath);
        }
    }else{
        printf("rmdir: can't delete /\n");
    }
}

void buildinTouch(uint32 argc, char** argv) {
    if (argc != 2) {
        printf("touch: only support 1 argument\n");
        return;
    }
    char finalPath[MAX_PATH_LEN] = {0};
    makeClearAbsPath(argv[1], finalPath);
    int fd = -1;
    if ((fd = open(finalPath, O_CREAT)) == -1) {
        printf("touch: create file %s failed\n", finalPath);
        return;
    }
    close(fd);
}

void buildinRm(uint32 argc, char** argv) {
    if (argc != 2) {
        printf("rm: only support 1 argument\n");
        return;
    }
    char finalPath[MAX_PATH_LEN] = {0};
    makeClearAbsPath(argv[1], finalPath);
    if (unlink(finalPath)== -1) {
        printf("rm: delete file %s failed\n", finalPath);
    }
}

void buildinWf(uint32 argc, char** argv) {
    if (argc != 3) {
        printf("wf: only support 2 argument\n");
        return;
    }
    char finalPath[MAX_PATH_LEN] = {0};
    makeClearAbsPath(argv[1], finalPath);
    int fd = -1;
    if ((fd = open(finalPath, O_WRONLY)) == -1) {
        printf("wf: open file %s failed\n", finalPath);
        return;
    }
    write(fd, argv[2], strlen(argv[2]));
    close(fd);
}

void buildinRf(uint32 argc, char** argv) {
    if (argc != 2) {
        printf("rf: only support 1 argument\n");
        return;
    }
    char finalPath[MAX_PATH_LEN] = {0};
    makeClearAbsPath(argv[1], finalPath);
    int fd = -1;
    if ((fd = open(finalPath, O_RDONLY)) == -1) {
        printf("rf: open file %s failed\n", finalPath);
        return;
    }
    char buf[1024];
    int cnt = -1;
    while ((cnt = read(fd, buf, 1024)) != -1) {
        write(1, buf, cnt);
    }
    close(fd);
}