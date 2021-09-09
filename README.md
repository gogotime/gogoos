# PyroOS
&nbsp;&nbsp;&nbsp;&nbsp;个人娱乐项目，参照《操作系统真象还原》这本书</br>
&nbsp;&nbsp;&nbsp;&nbsp;一个玩具操作系统，支持32位X86处理器，支持4GB虚拟内存</br>
&nbsp;&nbsp;&nbsp;&nbsp;X86汇编器使用nasm，C编译器使用clang</br>
&nbsp;&nbsp;&nbsp;&nbsp;基本实现了操作系统需要的进程管理，内存管理，文件管理，设备驱动等功能</br>
&nbsp;&nbsp;&nbsp;&nbsp;总计约7000行，可以运行外部编译好的程序

    进程管理：进/线程创建，调度，销毁；信号量，锁，管程等
    内存管理：页表地址映射；内存块/页分配，回收等
    文件管理：硬盘格式化，挂载；文件/目录创建，读写，删除；工作目录切换；Inode，文件描述符等
    设备驱动：屏幕，定时器，键盘，硬盘读写等
    数据结构：位图，双向链表，环形队列
    其他：简单shell及内嵌指令；简易crt库，lib库,ELF文件加载等
![img1](https://s3.bmp.ovh/imgs/2021/09/26e3fe2b9a879f8b.png)
## 演示
#### 简单shell指令
![img2](https://i.bmp.ovh/imgs/2021/09/29f15227b46e9349.png)
![img3](https://i.bmp.ovh/imgs/2021/09/c868c62e65f86a49.png)

#### 加载用户程序
    系统使用了两个硬盘，sda为系统启动盘，无格式
    sdb为实际使用挂载盘，有自己的文件格式
    故先将程序编译好，用自己的系统调用库，c运行时库链接，dd到sda裸盘某处
    再在系统启动时读硬盘，将其写入sdb中的文件系统并运行，即下图的t1


```
// 待加载程序
// userprog/test1.c

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

```

```C
// 从裸盘sda写到格式化盘sdb
// kernel/main.c

int main(){
    
    ...
    
    uint32 fileSize = 9136; // 根据编译出来的elf文件大小决定
    uint32 secCnt = DIV_ROUND_UP(fileSize, SECTOR_BYTE_SIZE);
    Disk* sda = &ideChannel[0].devices[0]; // 选定sda
    void* buf = sysMalloc(secCnt * SECTOR_BYTE_SIZE); //分配缓冲区内存
    if (buf == NULL) {
        printk("main: buf = sysMalloc(secCnt * SECTOR_BYTE_SIZE) failed\n");
        while (1);
    }
    ideRead(sda, 400, buf, secCnt); // 读硬盘
    int32 fd = sysOpen("/t1", O_CREAT | O_RDWR); // 创建文件

    if (fd == -1) {
        printk("create file test1 failed\n");
        while (1);
    }

    if (sysWrite(fd, buf, fileSize) == -1) { // 写文件
        printk("write file test1 failed\n");
        while (1);
    }

    sysClose(fd); // 释放资源
    sysFree(buf);
    
    ...
    
```
![img3](https://i.bmp.ovh/imgs/2021/09/1334b9d1a5e8b383.png)
