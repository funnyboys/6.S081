# 概要
文件系统的目的：
管理和存储数据、支持数据在用户和程序间共享、支持数据一致性(重启后仍然生效)

挑战：
* 文件系统结构、目录结构、文件信息等内容都需要保存在disk中。
* 支持crash recovery，文件系统损坏后重启可恢复
    * 文件系统的crash可能会导致disk上的数据不一致(例如：block被文件打开，却被标记为空闲状态)
* 提供并发支持多进程的访问
* 维护最常使用block的memory cache(访问disk比访问memory慢n个指数级)

# overview
## 分层
xv6的文件系统被分为7层，分别为：
| xv6文件系统层次结构 |
| :-: |
| file descriptor |
| pathname |
| directory |
| inode |
| logging |
| buffer cache |
| disk |

1. disk
    * 通过virtio硬件读写block数据
2. buffer cache
    * 缓存disk block
    * 并发控制，保证一个时刻只有一个内核进程可以访问某个block
3. logging layer
    * 事务(transaction)：将上层对多个block的操作打包成一次事务传输
    * 出现crash时，保证所有block的操作都是原子的
4. inode
    * 为每一个文件实现一个特殊的inode，inode-number保证独一无二
5. directory layer
    * 将每一个目录实现为特殊的inode
    * inode的内容为一系列的directory entry
    * 每一个directory entry都包含文件名和inode number
6. pathname
    * pathname层提供了分层的目录结构，通过递归实现
7. file descriptor
    * file descriptor层将unix的许多系统资源抽象为文件(pipe, devices, files)

## disk layout
1. block0
    * 保存boot sector
2. block1
    * 保存superblock
3. log
    * 从block2开始，共sb.nlog个block
4. inodes
    * 从block(2+sb.nlog)开始，共sb.ninodes个block
5. bitmap
    * inodes之后，共 FSSIZE/(BSIZE*8) + 1 个block
    * bitmap使用一个bit来标识当前的block是否使用
6. data block
    * bitmap之后剩余的所有block，都是data block

| 0    | 1     | 2   |  | |  |blockn | 
| :-: | :-: | :-: | :-: | :-: | :-: | :-: | :-: | 
| boot | super | log(多个block) | inodes(多个block) | bitmap(多个block) | data | ... | data |

# superblock
```
// Disk layout:
// [ boot block | super block | log | inode blocks | free bit map | data blocks]
//
// mkfs computes the super block and builds an initial file system. The
// super block describes the disk layout:
struct superblock {
    uint magic;        // Must be FSMAGIC
    uint size;         // Size of file system image (blocks)
    uint nblocks;      // Number of data blocks
    uint ninodes;      // Number of inodes.
    uint nlog;         // Number of log blocks
    uint logstart;     // Block number of first log block
    uint inodestart;   // Block number of first inode block
    uint bmapstart;    // Block number of first free map block
};
```
存储文件系统的metadata。
superblock的内容在制作文件系统时填充，具体可以参考mkfs.c。

# buffer cache layer
## 目的
buffer cache layer主要有两个目的：
1. 同步对disk的访问，保证任一时刻data block在内存中只有一份且只有一个进程在访问;
2. cache部分disk block，不需要从disk中读取，加速读操作。

## 重要数据结构
struct buf是disk block在内存中的缓存数据。
```
struct buf {
    int valid;   // has data been read from disk?
    int disk;    // does disk "own" buf?
    uint dev;
    uint blockno;
    struct sleeplock lock;
    uint refcnt;
    struct buf *prev; // LRU cache list
    struct buf *next;
    uchar data[BSIZE];
};
```
b->valid:
bget()函数中使用，用于标识当前block cache的数据是否有效：
* 0: block cache数据无效，需要从disk读
* 1: block cache数据有效

## 主要接口
### binit

### bread
```
struct buf* bread(uint dev, uint blockno)
```
输入设备号和blockno，读取block中的数据到内存中的缓存struct buf。

### bwrite
```
void bwrite(struct buf *b)
```
将内存中缓存的block，写回到disk中。

### brelse
```
void brelse(struct buf *b)
```
减小该buf的引用，如果buf->refcnt = 0，释放该buf。

# logging layer
## 设计思路
allow higher layers to wrap updates to several blocks in a transaction.

## log层的物理结构
block0 -> block1 -> block2 -> ... -> blockn

1. block0
    * 存储 logheader
2. block1 -> blockn
    * 缓存的每个block数据

| 0    | 1 | | blockn | 
| :-: | :-: | :-: | :-: |
| logheader | data | ... | data |

* logheader
logheader保存了log层中缓存的所有block信息。
logheader->n：log中已经缓存的block数
logheader->block[i]：缓存的block在disk中对应的blockno
```
struct logheader {
    int n;                // log中已经保存的block个数
    int block[LOGSIZE];   // 缓存的 blockno 编号
};
struct log {
    struct spinlock lock;
    int start;
    int size;
    int outstanding; // how many FS sys calls are executing.
    int committing;  // in commit(), please wait.
    int dev;
    struct logheader lh;
};
```

## 代码典型调用流
一般情况下，log层的代码这样使用：
```
begin_op();
...
bp = bread(...);
bp->data[...] = ...;
log_write(bp);
...
end_op();
```

## 主要接口
### begin_op
```
void begin_op()
```
1. 等待当前log层已经ready
    * 当前没有log正在提交(log.committing == 0)
    * log层有空余的空间存储缓存(log.lh.n + (log.outstanding+1)*MAXOPBLOCKS > LOGSIZE)
2. 增加log.outstanding计数(how many FS sys calls are executing)

### log_write
```
void log_write(struct buf *b)
```
将当前buf的blockno保存到log的memory缓存中(logheader in memory)。

### end_op
```
void end_op(void)
```
1. 减去当前outstanding计数
2. 如果outstanding为0，开始commit
    * commit()
        1. write_log()
            1. 遍历log.lh.block[]中缓存的每一个blockno
            2. 读取每一个blockno对应的数据
            3. 将数据写入到log.start开始的每个block中(disk)
        2. write_head()
            * 更新log.lh.block[]中缓存的blockno到log层第一个block(disk0 - blockheader)
        3. install_trans()
            1. 从log.start开始遍历每一个block在log disk上的缓存
            2. 将log disk上的缓存更新到真正的block中(真正的blockno)
        4. log.lh.n = 0
            * 数据已经更新完毕，log在memory中的缓存置为零
        5. write_head()
            * 将log为零写入disk
3. 如果outstanding不为0，wakeup在begin_op()中睡眠的进程

### recover_from_log
```
void recover_from_log(void)
```
在启动时调用：`initlog() -> recover_from_log() `
1. read_head()读取log层在disk中的文件；
2. 如果有需要恢复的数据，install_trans()恢复上次数据；
3. 将log在memory中的缓存置为零。

# bitmap
## 思路
bitmap存储在disk中，使用一个bit来标识当前的block是否使用。
bitmap存储在inodes之后，共 FSSIZE/(BSIZE*8) + 1 个block

# inode
## inode的两个含义
on-disk数据结构：包含文件的size和data block number链表。
in-memory inode：on-disk inode的副本和内核需要的部分信息。

## on-disk inode
on-disk inode保存在硬盘的inode分区，从block(2+sb.nlog)开始，共sb.ninodes个block。
xv6使用 struct dinode 保存 on-disk 的 inode 信息。

## in-memory inode
struct inode 用来表示 in-memory inode 。