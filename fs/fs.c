#include "../device/ide.h"
#include "../device/console.h"
#include "../kernel/global.h"
#include "../kernel/memory.h"
#include "fs.h"
#include "dir.h"
#include "super_block.h"
#include "file.h"
#include "../lib/debug.h"
#include "../lib/kernel/stdio.h"
#include "../lib/string.h"
#include "../lib/structure/ioqueue.h"

#define SUPER_BLOCK_MAGIC_NUM 0x20212021

Partition* curPart;
extern List partitionList;
extern Dir rootDir;
extern File fileTable[MAX_FILE_OPEN_ALL];
extern IOQueue keyboardBuf;

void partitionFormat(Partition* partition) {
    uint32 bootSecCnt = 1;
    uint32 superBlockSecCnt = 1;
    uint32 inodeBitMapSecCnt = DIV_ROUND_UP(MAX_FILE_PER_PART, SECTOR_BYTE_SIZE * 8);
    uint32 inodeTableSecCnt = DIV_ROUND_UP(sizeof(Inode) * MAX_FILE_PER_PART, SECTOR_BYTE_SIZE);
    uint32 usedSecCnt = bootSecCnt + superBlockSecCnt + inodeBitMapSecCnt + inodeTableSecCnt;
    uint32 freeSecCnt = partition->secCnt - usedSecCnt;

    uint32 blockBitMapSecCnt = DIV_ROUND_UP(freeSecCnt, BITS_PER_SECTOR);
    uint32 blockBitMapBitCnt = freeSecCnt - blockBitMapSecCnt;
    blockBitMapSecCnt = DIV_ROUND_UP(blockBitMapBitCnt, BITS_PER_SECTOR);

    SuperBlock sb;
    ASSERT(sizeof(SuperBlock) == SECTOR_BYTE_SIZE)
    sb.magicNum = SUPER_BLOCK_MAGIC_NUM;
    sb.secCnt = partition->secCnt;
    sb.inodeCnt = MAX_FILE_PER_PART;
    sb.partLbaStart = partition->lbaStart;

    sb.blockBitMapLbaStart = sb.partLbaStart + 2;
    sb.blockBitMapSecCnt = blockBitMapSecCnt;

    sb.inodeBitMapLbaStart = sb.blockBitMapLbaStart + sb.blockBitMapSecCnt;
    sb.inodeBitMapSecCnt = inodeBitMapSecCnt;

    sb.inodeTableLbaStart = sb.inodeBitMapLbaStart + sb.inodeBitMapSecCnt;
    sb.inodeTableSecCnt = inodeTableSecCnt;

    sb.dataLbaStart = sb.inodeTableLbaStart + inodeTableSecCnt;
    sb.rootInodeNo = 0;
    sb.dirEntrySize = sizeof(DirEntry);

    printk("    %s info:\n", partition->name);
    printk("        secCnt:%d\n", sb.secCnt);
    printk("        partLbaStart:%x\n", sb.partLbaStart);
    printk("        blockBitMapLbaStart:%x\n", sb.blockBitMapLbaStart);
    printk("        blockBitMapSecCnt:%d\n", sb.blockBitMapSecCnt);
    printk("        inodeBitMapLbaStart:%x\n", sb.inodeBitMapLbaStart);
    printk("        dataLbaStart:%x\n", sb.dataLbaStart);

    Disk* hd = partition->disk;
    ideWrite(hd, partition->lbaStart + 1, &sb, 1);
    uint32 bufSize = (sb.blockBitMapSecCnt >= sb.inodeBitMapSecCnt ? sb.blockBitMapSecCnt : sb.inodeBitMapSecCnt);
    bufSize = (bufSize >= sb.inodeTableSecCnt ? bufSize : sb.inodeTableSecCnt) * SECTOR_BYTE_SIZE;
    uint8* buf = (uint8*) sysMalloc(bufSize);
    printk("    buf size:%d\n", bufSize);
    buf[0] |= 0x01;
    uint32 blockBitMapLastByte = blockBitMapBitCnt / 8;
    uint32 blockBitMapLastBit = blockBitMapBitCnt % 8;
    uint32 lastSize = SECTOR_BYTE_SIZE - (blockBitMapLastByte % SECTOR_BYTE_SIZE);

    memset(&buf[blockBitMapLastByte], 0xff, lastSize);

    uint8 bitIdx = 0;
    while (bitIdx < blockBitMapLastBit) {
        buf[blockBitMapLastByte] &= ~(1 << bitIdx++);
    }
    ideWrite(hd, sb.blockBitMapLbaStart, buf, sb.blockBitMapSecCnt);

    memset(buf, 0, bufSize);
    buf[0] |= 0x01;
    ideWrite(hd, sb.inodeBitMapLbaStart, buf, sb.inodeBitMapSecCnt);

    memset(buf, 0, bufSize);
    Inode* i = (Inode*) buf;
    i->size = sb.dirEntrySize * 2; //. and ..
    i->ino = 0;
    i->block[0] = sb.dataLbaStart;
    ideWrite(hd, sb.inodeTableLbaStart, buf, sb.inodeTableSecCnt);

    memset(buf, 0, bufSize);
    DirEntry* dirEntry = (DirEntry*) buf;
    memcpy(dirEntry->fileName, ".", 1);
    dirEntry->ino = 0;
    dirEntry->fileType = FT_DIRECTORY;
    dirEntry++;

    memcpy(dirEntry, "..", 2);
    dirEntry->ino = 0;
    dirEntry->fileType = FT_DIRECTORY;

    ideWrite(hd, sb.dataLbaStart, buf, 1);

    printk("    %s format done\n", partition->name);
    sysFree(buf);
}

static bool mountPartition(ListElem* elem, int arg) {
    char* partName = (char*) arg;
    Partition* partition = elemToEntry(Partition, partTag, elem);
    if (strcmp(partition->name, partName)) {
        return false;
    }
    curPart = partition;
    Disk* hd = partition->disk;
    SuperBlock* buf = (SuperBlock*) sysMalloc(SECTOR_BYTE_SIZE);
    curPart->superBlock = (SuperBlock*) sysMalloc(sizeof(SuperBlock));
    if (curPart->superBlock == NULL) {
        PANIC("malloc curPart->superBlock failed")
    }
    memset(buf, 0, SECTOR_BYTE_SIZE);
    ideRead(hd, curPart->lbaStart + 1, buf, 1);
    memcpy(curPart->superBlock, buf, sizeof(SuperBlock));
    curPart->blockBitMap.startAddr = (uint8*) sysMalloc(buf->blockBitMapSecCnt * SECTOR_BYTE_SIZE);
    if (curPart->blockBitMap.startAddr == NULL) {
        PANIC("malloc curPart->blockBitMap.startAddr failed")
    }
    curPart->blockBitMap.length = buf->blockBitMapSecCnt * SECTOR_BYTE_SIZE;
    ideRead(hd, buf->blockBitMapLbaStart, curPart->blockBitMap.startAddr, buf->blockBitMapSecCnt);

    curPart->inodeBitMap.startAddr = (uint8*) sysMalloc(buf->inodeBitMapSecCnt * SECTOR_BYTE_SIZE);
    if (curPart->inodeBitMap.startAddr == NULL) {
        PANIC("malloc curPart->inodeBitMap.startAddr failed")
    }
    curPart->inodeBitMap.length = buf->inodeBitMapSecCnt * SECTOR_BYTE_SIZE;
    ideRead(hd, buf->inodeBitMapLbaStart, curPart->inodeBitMap.startAddr, buf->inodeBitMapSecCnt);

    listInit(&curPart->openInodes);
    printk("mount %s done\n", partition->name);

    return true;
}

char* pathParse(char* pathName, char* nameStore) {
    if (pathName[0] == '/') {
        while (*(++pathName) == '/');
    }
    while (*pathName != '/' && *pathName != 0) {
        *nameStore++ = *pathName++;
    }
    if (pathName[0] == 0) {
        return NULL;
    }
    return pathName;
}

int32 pathDepthCnt(const char* pathName) {
    ASSERT(pathName != NULL)
    char* p = (char*) pathName;
    char name[MAX_FILE_NAME_LEN];
    uint32 depth = 0;
    p = pathParse(p, name);
    while (name[0]) {
        depth++;
        memset(name, 0, MAX_FILE_NAME_LEN);
        if (p) {
            p = pathParse(p, name);
        }
    }
    return depth;
}

int searchFile(const char* pathName, PathSearchRecord* record) {
    if (!strcmp(pathName, "/") || !strcmp(pathName, "/.") || !strcmp(pathName, "/..")) {
        record->parentDir = &rootDir;
        record->fileType = FT_DIRECTORY;
        record->searchPath[0] = 0;
        return 0;
    }
    uint32 pathLen = strlen(pathName);
    ASSERT(pathName[0] == '/' && pathLen > 0 && pathLen < MAX_PATH_LEN)
    char* subPath = (char*) pathName;
    Dir* parentDir = &rootDir;
    DirEntry de;
    char name[MAX_FILE_NAME_LEN] = {0};
    record->parentDir = parentDir;
    record->fileType = FT_UNKNOWN;
    uint32 parentIno = 0;
    subPath = pathParse(subPath, name);
    while (name[0]) {
        ASSERT(strlen(record->searchPath) < 512)
        strcat(record->searchPath, "/");
        strcat(record->searchPath, name);
        if (searchDirEntry(curPart, parentDir, name, &de)) {
            memset(name, 0, MAX_FILE_NAME_LEN);
            if (subPath) {
                subPath = pathParse(subPath, name);
            }
            if (de.fileType == FT_DIRECTORY) {
                parentIno = parentDir->inode->ino;
                if (parentDir != &rootDir) {
                    dirClose(parentDir);
                }
                parentDir = dirOpen(curPart, de.ino);
                record->parentDir = parentDir;
            } else if (de.fileType == FT_REGULAR) {
                record->fileType = FT_REGULAR;
                return de.ino;
            }
        } else {
            return -1;
        }
    }
    dirClose(record->parentDir);
    record->parentDir = dirOpen(curPart, parentIno);
    record->fileType = FT_DIRECTORY;
    return de.ino;
}

static uint32 fdLocalToGlobal(uint32 localFd) {
    TaskStruct* cur = getCurrentThread();
    int32 globalFd = cur->fdTable[localFd];
    ASSERT(globalFd >= 0 && globalFd < MAX_FILE_OPEN_ALL)
    return (uint32) globalFd;
}

int32 sysOpen(const char* pathName, OFlags flags) {
    if (pathName[strlen(pathName) - 1] == '/') {
        printk("can't open directory %s with open(), use openDir() instead\n", pathName);
        return -1;
    }
    ASSERT(flags <= 7)
    int32 fd = -1;
    PathSearchRecord record;
    memset(&record, 0, sizeof(record));
    uint32 pathNameDepth = pathDepthCnt(pathName);
    int ino = searchFile(pathName, &record);
    bool found = (ino != -1) ? true : false;
    if (record.fileType == FT_DIRECTORY) {
        printk("can't open directory %s with open(), use openDir() instead\n", pathName);
        dirClose(record.parentDir);
        return -1;
    }
    uint32 pathSearchedDepth = pathDepthCnt(record.searchPath);
    if (pathNameDepth != pathSearchedDepth) {
        printk("cannot access %s: Not a directory, subpath %s isn't exist\n", pathName, record.searchPath);
        dirClose(record.parentDir);
        return -1;
    }
    char* fileName = strrchr(record.searchPath, '/') + 1;
    if (!found && !(flags & O_CREAT)) {
        printk("in path:%s, file %s isn't exist", pathName, fileName);
        dirClose(record.parentDir);
        return -1;
    } else if (found && (flags & O_CREAT)) {
        printk("%s already exist!\n", pathName);
        dirClose(record.parentDir);
        return -1;
    }
    switch (flags & O_CREAT) {
        case O_CREAT:
//            printk("creating file\n");
            fd = fileCreate(record.parentDir, fileName, flags);
            dirClose(record.parentDir);
            break;
        default:
//            printk("sysOpen no create\n");
            fd = fileOpen(ino, flags);
            dirClose(record.parentDir);
    }
    return fd;
}

int32 sysClose(int32 fd) {
    int32 ret = 1;
    if (fd > 2) {
        uint32 globalFd = fdLocalToGlobal(fd);
        ret = fileClose(&fileTable[globalFd]);
        getCurrentThread()->fdTable[fd] = -1;
    }
    return ret;
}

uint32 sysWrite(int32 fd, const void* buf, uint32 count) {
    if (fd < 0) {
        printk("sysWrite error: fd<0\n");
        return -1;
    }
    if (fd == stdout) {
        char tmpBuf[1024] = {0};
        memcpy(tmpBuf, buf, count);
        ASSERT(count <= 1024)
        consolePutString(tmpBuf);
        return count;
    }
    uint32 globalFd = fdLocalToGlobal(fd);
    File* wf = &fileTable[globalFd];
    if (wf->fdFlag & O_WRONLY || wf->fdFlag & O_RDWR) {
        uint32 bytesWritten = fileWrite(wf, buf, count);
        return bytesWritten;
    } else {
        consolePutString("sysWrite: not allowed to write\n");
        return -1;
    }
}

uint32 sysRead(int32 fd, const void* buf, uint32 count) {
    ASSERT(buf != NULL)
    int32 ret = -1;
    if (fd < 0 || fd == stdout || fd == stderr) {
        printk("sysRead error: fd < 0 || fd == stdout || fd == stderr\n");
        return -1;
    } else if (fd == stdin) {
        char* dst =(char *) buf;
        uint32 bytesRead = 0;
        while (bytesRead < count) {
            *dst = ioQueueGetChar(&keyboardBuf);
            bytesRead++;
            dst++;
        }
        ret = (bytesRead == 0) ? -1 : (int32) bytesRead;
    }else{
        uint32 globalFd = fdLocalToGlobal(fd);
        ret = fileRead(&fileTable[globalFd], buf, count);
    }
    return ret;
}

int32 sysLseek(int32 fd, int32 offset, uint8 seekFlag) {
    if (fd < 0) {
        printk("sysLseek error: fd<0\n");
        return -1;
    }
    ASSERT(seekFlag > 0 && seekFlag < 4)
    uint32 globalFd = fdLocalToGlobal(fd);
    File* file = &fileTable[globalFd];
    int32 newPos = 0;
    int32 fileSize = (int32) file->fdInode->size;
    switch (seekFlag) {
        case SEEK_SET:
            newPos = offset;
            break;
        case SEEK_CUR:
            newPos = (int32) file->fdPos + offset;
            break;
        case SEEK_END:
            newPos = fileSize + offset;
            break;
    }
    if (newPos < 0 || newPos > (fileSize - 1)) {
        return -1;
    }
    file->fdPos = newPos;
    return file->fdPos;
}

int32 sysUnlink(const char* pathName) {
    ASSERT(strlen(pathName) < MAX_PATH_LEN)
    PathSearchRecord record;
    memset(&record, 0, sizeof(PathSearchRecord));
    int ino = searchFile(pathName, &record);
    ASSERT(ino != 0)
    if (ino == -1) {
        printk("file %s not found!\n", pathName);
        dirClose(record.parentDir);
        return -1;
    }
    if (record.fileType == FT_DIRECTORY) {
        printk("can't delete a directory with unlink(), use rmdir() instead\n");
        dirClose(record.parentDir);
        return -1;
    }
    uint32 fileIdx = 0;
    while (fileIdx < MAX_FILE_OPEN_ALL) {
        if (fileTable[fileIdx].fdInode != NULL && fileTable[fileIdx].fdInode->ino == ino) {
            break;
        }
        fileIdx++;
    }
    if (fileIdx < MAX_FILE_OPEN_ALL) {
        printk("file %s is in use, not allowed to delete\n", pathName);
        dirClose(record.parentDir);
        return -1;
    }
    ASSERT(fileIdx == MAX_FILE_OPEN_ALL)
    void* ioBuf = sysMalloc(2 * SECTOR_BYTE_SIZE);
    if (ioBuf == NULL) {
        printk("sysUnlink: ioBuf = sysMalloc(2 * SECTOR_BYTE_SIZE) failed\n");
        dirClose(record.parentDir);
        return -1;
    }
    Dir* parentDir = record.parentDir;
    deleteDirEntry(curPart, parentDir, ino, ioBuf);
    inodeRelease(curPart, ino);
    sysFree(ioBuf);
    dirClose(record.parentDir);
    return 0;
}

int32 sysMkdir(const char* pathName) {
    uint8 rollBackStep = 0;
    void* ioBuf = sysMalloc(SECTOR_BYTE_SIZE * 2);
    if (ioBuf == NULL) {
        printk("sysMkdir: ioBuf = sysMalloc(SECTOR_BYTE_SIZE * 2) failed\n", pathName);
        return -1;
    }
    PathSearchRecord record;
    memset(&record, 0, sizeof(PathSearchRecord));
    int ino = searchFile(pathName, &record);
    ASSERT(ino != 0)
    if (ino != -1) {
        printk("sysMkdir: file or directory %s already exist!\n", pathName);
        rollBackStep = 1;
        goto rollback;
    } else {
        uint32 pathNameDepth = pathDepthCnt(pathName);
        uint32 pathSearchedDepth = pathDepthCnt(record.searchPath);
        if (pathNameDepth != pathSearchedDepth) {
            printk("sysMkdir: can't make %s, subpath %s isn't exist!\n", pathName, record.searchPath);
            rollBackStep = 1;
            goto rollback;
        }
    }

    Dir* parentDir = record.parentDir;
    char* dirName = strrchr(record.searchPath, '/') + 1;
    ino = inodeBitMapAlloc(curPart);
    if (ino == -1) {
        printk("sysMkdir: ino = inodeBitMapAlloc(curPart) failed\n");
        rollBackStep = 1;
        goto rollback;
    }
    Inode newDirInode;
    inodeInit(ino, &newDirInode);
    uint32 blockBitMapIdx = 0;
    int32 blockLba = -1;
    blockLba = blockBitMapAlloc(curPart);
    if (blockLba == -1) {
        printk("sysMkdir: blockLba = blockBitMapAlloc(curPart) failed\n");
        rollBackStep = 2;
        goto rollback;
    }

    newDirInode.block[0] = blockLba;
    blockBitMapIdx = blockLbaToBitMapIdx(blockLba);
    ASSERT(blockBitMapIdx != 0)
    bitMapSync(curPart, blockBitMapIdx, BLOCK_BITMAP);

    memset(ioBuf, 0, 2 * SECTOR_BYTE_SIZE);
    DirEntry* curDe = (DirEntry*) ioBuf;

    memcpy(curDe->fileName, ".", 1);
    curDe->ino = ino;
    curDe->fileType = FT_DIRECTORY;
    curDe++;

    memcpy(curDe->fileName, "..", 2);
    curDe->ino = record.parentDir->inode->ino;
    curDe->fileType = FT_DIRECTORY;

    ideWrite(curPart->disk, newDirInode.block[0], ioBuf, 1);

    newDirInode.size = 2 * curPart->superBlock->dirEntrySize;

    DirEntry newDirEntry;
    memset(&newDirEntry, 0, sizeof(DirEntry));
    createDirEntry(dirName, ino, FT_DIRECTORY, &newDirEntry);
    memset(ioBuf, 0, 2 * SECTOR_BYTE_SIZE);
    if (!syncDirEntry(parentDir, &newDirEntry, ioBuf)) {
        printk("sysMkdir: syncDirEntry(parentDir, &newDirEntry, ioBuf) failed\n");
        rollBackStep = 2;
        goto rollback;
    }

    memset(ioBuf, 0, 2 * SECTOR_BYTE_SIZE);
    inodeSync(curPart, parentDir->inode, ioBuf);

    memset(ioBuf, 0, 2 * SECTOR_BYTE_SIZE);
    inodeSync(curPart, &newDirInode, ioBuf);

    bitMapSync(curPart, ino, INODE_BITMAP);

    sysFree(ioBuf);
    dirClose(record.parentDir);
    return 0;

    rollback:
    switch (rollBackStep) {
        case 2:
            bitMapSet(&curPart->inodeBitMap, ino, 0);
            break;
        case 1:
            dirClose(record.parentDir);
            break;
    }
    sysFree(ioBuf);
    return -1;
}

Dir* sysOpenDir(const char* pathName) {
    ASSERT(strlen(pathName) <= MAX_PATH_LEN)
    if (pathName[0] == '/' && (pathName[1] == 0 || pathName[1] == '.')) {
        return &rootDir;
    }
    PathSearchRecord record;
    memset(&record, 0, sizeof(PathSearchRecord));
    int32 ino = searchFile(pathName, &record);
    Dir* ret = NULL;
    if (ino == -1) {
//        printk("sysOpenDir: directory %s isn't existing\n", pathName);
    } else {
        if (record.fileType == FT_REGULAR) {
//            printk("sysOpenDir: %s is a file, not directory\n", pathName);
        } else if (record.fileType == FT_DIRECTORY) {
            ret = dirOpen(curPart, ino);
        }
    }
    return ret;
}

int32 sysCloseDir(Dir* dir) {
    int32 ret = -1;
    if (dir != NULL) {
        dirClose(dir);
        ret = 0;
    }
    return ret;
}

DirEntry* sysReadDir(Dir* dir) {
    ASSERT(dir != NULL)
    return dirRead(dir);
}

void sysRewindDir(Dir* dir) {
    ASSERT(dir != NULL)
    dir->dirPos = 0;
}

int32 sysRmdir(const char* pathName) {
    PathSearchRecord record;
    memset(&record, 0, sizeof(PathSearchRecord));
    int32 ino = searchFile(pathName, &record);
    int32 ret = -1;
    if (ino == -1) {
        printk("sysRmdir: directory %s isn't existing\n", pathName);
    } else if (ino == 0) {
        printk("sysRmdir: can't delete root directory\n");
    } else {
        Dir* dir = dirOpen(curPart, ino);
        if (!dirIsEmpty(dir)) {
            printk("sysRmdir: can't remove directory, %s isn't empty\n", pathName);
        } else {
            if (!dirRemove(record.parentDir, dir)) {
                ret = 0;
            }
        }
        dirClose(dir);
    }
    dirClose(record.parentDir);
    return ret;
}

static uint32 getParentDirIno(uint32 childIno, void* ioBuf) {
    Inode* childDirInode = inodeOpen(curPart, childIno);
    uint32 blockLba = childDirInode->block[0];
    ASSERT(blockLba >= curPart->superBlock->dataLbaStart)
    inodeClose(childDirInode);
    ideRead(curPart->disk, blockLba, ioBuf, 1);
    DirEntry* de = (DirEntry*) ioBuf;
    ASSERT(de[1].ino < 4096 && de[1].fileType == FT_DIRECTORY)
    return de[1].ino;
}

static int32 getChildDirName(uint32 parentIno, uint32 childIno, char* path, void* ioBuf) {
    Inode* parentDirInode = inodeOpen(curPart, parentIno);
    uint8 blockIdx = 0;
    uint32 blockCnt = 12;
    uint32* allBlocks = (uint32*) sysMalloc(560);
    if (allBlocks == NULL) {
        printk("getChildDirName: allBlocks = (uint32*) sysMalloc(560); failed");
        return -1;
    }
    while (blockIdx < 12) {
        allBlocks[blockIdx] = parentDirInode->block[blockIdx];
        blockIdx++;
    }
    if (parentDirInode->block[12] != 0) {
        ideRead(curPart->disk, parentDirInode->block[12], allBlocks + 12, 1);
        blockCnt = 140;
    }
    inodeClose(parentDirInode);
    blockIdx = 0;
    uint32 curDirEntryPos = 0;
    uint32 dirEntryIdx = 0;
    uint32 dirEntrySize = curPart->superBlock->dirEntrySize;
    uint32 dirEntryCntPerSec = SECTOR_BYTE_SIZE / dirEntrySize;
    DirEntry* de = (DirEntry*) ioBuf;
    while (blockIdx < blockCnt) {
        if (allBlocks[blockIdx] == 0) {
            blockIdx++;
            continue;
        }
        memset(de, 0, SECTOR_BYTE_SIZE);
        ideRead(curPart->disk, allBlocks[blockIdx], de, 1);
        dirEntryIdx = 0;
        while (dirEntryIdx < dirEntryCntPerSec) {
            if ((de + dirEntryIdx)->ino == childIno) {
                strcat(path, "/");
                strcat(path, (de + dirEntryIdx)->fileName);
                sysFree(allBlocks);
                return 0;
            }
            dirEntryIdx++;
        }
        blockIdx++;
    }
    sysFree(allBlocks);
    return -1;
}

char* sysGetCwd(char* buf, uint32 size) {
    ASSERT(buf != NULL)
    void* ioBuf = sysMalloc(SECTOR_BYTE_SIZE);
    if (ioBuf == NULL) {
        return NULL;
    }
    TaskStruct* cur = getCurrentThread();
    int32 parentIno = 0;
    int32 childIno = cur->cwdIno;
    ASSERT(childIno >= 0 && childIno < 4096)
    if (childIno == 0) {
        buf[0] = '/';
        buf[1] = 0;
        return buf;
    }
    char filePathReverse[MAX_PATH_LEN] = {0};
    while (childIno != 0) {
        parentIno = getParentDirIno(childIno, ioBuf);
        if (getChildDirName(parentIno, childIno, filePathReverse, ioBuf) == -1) {
            sysFree(ioBuf);
            return NULL;
        }
        childIno = parentIno;
    }
    memset(buf, 0, size);
    ASSERT(strlen(filePathReverse) <= size)
    char* lastSlash;
    while ((lastSlash = strrchr(filePathReverse, '/'))) {
        uint16 len = strlen(buf);
        strcpy(buf + len, lastSlash);
        *lastSlash = 0;
    }
    sysFree(ioBuf);
    return buf;

}

int32 sysChDir(const char* pathName) {
    int32 ret = -1;
    PathSearchRecord record;
    memset(&record, 0, sizeof(PathSearchRecord));
    int32 ino = searchFile(pathName, &record);
    if (ino != -1) {
        if (record.fileType == FT_DIRECTORY) {
            getCurrentThread()->cwdIno = ino;
            ret = 0;
        } else {
            printk("sysChDir: %s is a file, not directory\n");
        }
    }
    dirClose(record.parentDir);
    return ret;
}

int32 sysStat(const char* path, Stat* stat) {
    if (!strcmp(path, "/") || !strcmp(path, "/.") || !strcmp(path, "/..")) {
        stat->fileType = FT_DIRECTORY;
        stat->ino = 0;
        stat->size = rootDir.inode->size;
        return 0;
    }
    int32 ret = -1;
    PathSearchRecord record;
    memset(&record, 0, sizeof(PathSearchRecord));
    int ino = searchFile(path, &record);
    if (ino != -1) {
        Inode* inode = inodeOpen(curPart, ino);
        stat->size = inode->size;
        inodeClose(inode);
        stat->fileType = record.fileType;
        stat->ino = ino;
        ret = 0;
    } else {
//        printk("sysStat: %s not found\n", path);
    }
    dirClose(record.parentDir);
    return ret;
}

void fsInit() {
    ASSERT(sizeof(SuperBlock) == SECTOR_BYTE_SIZE)
    SuperBlock* sb = (SuperBlock*) sysMalloc(SECTOR_BYTE_SIZE);
    ListElem* elem = partitionList.head.next;
    while (elem != &partitionList.tail) {
        Partition* partition = elemToEntry(Partition, partTag, elem);
        ideRead(partition->disk, partition->lbaStart + 1, sb, 1);
        if (sb->magicNum == SUPER_BLOCK_MAGIC_NUM) {
            printk("%s has fileSystem\n", partition->name);
            partitionFormat(partition);
        } else {
            printk("format %s\n", partition->name);
            partitionFormat(partition);
        }
        elem = elem->next;
    }
    char defaultPart[8] = "sdb1";
    listTraversal(&partitionList, mountPartition, (int) defaultPart);
    openRootDir(curPart);
    uint32 fdIdx = 0;
    while (fdIdx < MAX_FILE_OPEN_ALL) {
        fileTable[fdIdx].fdInode = NULL;
        fdIdx++;
    }
}