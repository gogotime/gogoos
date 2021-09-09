#include "../lib/user/stdio.h"

int main(int argc, char** argv) {
    printf("t1 from disk\n");
    printf("argc:%d\n", argc);
    int i = 0;
    while (i < argc) {
        printf("argc[%d]=%s\n", i, argv[i]);
        i++;
    }
    return 0;
}