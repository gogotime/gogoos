# ifndef __SHELL_BUILDINCMD_H
# define __SHELL_BUILDINCMD_H

void makeClearAbsPath(char* path, char* finalPath);

void buildinPwd(uint32 argc, char** argv);

int32 buildinCd(uint32 argc, char** argv);

void buildinLs(uint32 argc, char** argv);

void buildinPs(uint32 argc, char** argv);

void buildinClear(uint32 argc, char** argv);

void buildinMkdir(uint32 argc, char** argv);

void buildinRmdir(uint32 argc, char** argv);

void buildinTouch(uint32 argc, char** argv);

void buildinRm(uint32 argc, char** argv);

void buildinWf(uint32 argc, char** argv);

void buildinRf(uint32 argc, char** argv);
# endif