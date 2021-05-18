# pipe
```
int pipe(int*);

int fd[2];
ret = pipe(fd);
fd[0]: 读pipe fd
fd[1]: 写pipe fd
```
pipe()的参数为一个数组，如果函数执行成功，则：
fd[0]为读pipe的fd，fd[1]为写pipe的fd。

# fstat
` int fstat(int fd, struct stat *buf); `
## 描述
获取fd指向文件的信息(不需要拥有文件的权限)。
在stat()和lstat()的情况下，需要对指向该文件的路径中所有目录拥有execute(search)权限。
## stat数据结构
```
struct stat {
    dev_t     st_dev;         /* ID of device containing file */
    ino_t     st_ino;         /* inode number */
    mode_t    st_mode;        /* protection */
    nlink_t   st_nlink;       /* number of hard links */
    uid_t     st_uid;         /* user ID of owner */
    gid_t     st_gid;         /* group ID of owner */
    dev_t     st_rdev;        /* device ID (if special file) */
    off_t     st_size;        /* total size, in bytes */
    blksize_t st_blksize;     /* blocksize for filesystem I/O */
    blkcnt_t  st_blocks;      /* number of 512B blocks allocated */

    /* Since Linux 2.6, the kernel supports nanosecond
    precision for the following timestamp fields.
    For the details before Linux 2.6, see NOTES. */

    struct timespec st_atim;  /* time of last access */
    struct timespec st_mtim;  /* time of last modification */
    struct timespec st_ctim;  /* time of last status change */

    #define st_atime st_atim.tv_sec      /* Backward compatibility */
    #define st_mtime st_mtim.tv_sec
    #define st_ctime st_ctim.tv_sec
};
```
## stst->st_ino
st_ino包括两部分，文件类型和文件权限。
### 文件类型
通过掩码 0170000 来确定当前的文件类型，具体如下:
```
The following mask values are defined for the file type of the st_mode field:

    S_IFMT     0170000   bit mask for the file type bit field

    S_IFSOCK   0140000   socket
    S_IFLNK    0120000   symbolic link
    S_IFREG    0100000   regular file
    S_IFBLK    0060000   block device
    S_IFDIR    0040000   directory
    S_IFCHR    0020000   character device
    S_IFIFO    0010000   FIFO
```
同时，可以通过如下的宏来判断当前文件是否为某种文件类型：
```
S_ISREG(m)  is it a regular file?

S_ISDIR(m)  directory?

S_ISCHR(m)  character device?

S_ISBLK(m)  block device?

S_ISFIFO(m) FIFO (named pipe)?

S_ISLNK(m)  symbolic link?  (Not in POSIX.1-1996.)

S_ISSOCK(m) socket?  (Not in POSIX.1-1996.)
```
### 文件权限
文件权限通过12位掩码 0777 来确定。
三个7分别对应三种角色：user、group、other
7对应3个bit的权限，分别为: read, write, execute
## 实例
```
huangchangwei@ubuntu:~/bin$ ls -lh ctags
-rwxrwxr-x 1 huangchangwei huangchangwei 950K Mar  3  2018 ctags
```

# struct dirent
```
/* glibc-2.29/sysdeps/unix/sysv/linux/bits/dirent.h */
struct dirent
  {
#ifndef __USE_FILE_OFFSET64
    __ino_t d_ino;
    __off_t d_off;
#else
    __ino64_t d_ino;
    __off64_t d_off;
#endif
    unsigned short int d_reclen;
    unsigned char d_type;
    char d_name[256];       /* We must not include limits.h! */
  };
```
d_type的定义如下：
```
/* glibc-2.29/dirent/dirent.h */
/* File types for `d_type'.  */
enum
  {
    DT_UNKNOWN = 0,
# define DT_UNKNOWN DT_UNKNOWN
    DT_FIFO = 1,
# define DT_FIFO    DT_FIFO
    DT_CHR = 2,
# define DT_CHR     DT_CHR
    DT_DIR = 4,
# define DT_DIR     DT_DIR
    DT_BLK = 6,
# define DT_BLK     DT_BLK
    DT_REG = 8,
# define DT_REG     DT_REG
    DT_LNK = 10,
# define DT_LNK     DT_LNK
    DT_SOCK = 12,
# define DT_SOCK    DT_SOCK
    DT_WHT = 14
# define DT_WHT     DT_WHT
  };
```

# grep中的正则表达式
## 注意
grep中使用的语法为正则表达式，和shell中的语法不同，不要搞混淆。
例如：
```
*在shell中表示匹配任意长度的任意字符
 在grep中表示匹配前面的字符零次或多次
```
## ^ 锚头，匹配输入行的开始
^锚定行的开始，如 `^grep` 匹配所有以 grep 开始的行。

例如：

```
huangchangwei@ubuntu:~/svn_rep/linux$ grep -nr char ls.c
15:char *fmtname(char *path)
17:    static char buf[DIRSIZ+1];
18:    char *p;
20:    // Find first character after last slash.
37:        case S_IFCHR:  printf("character device\n");        break;
47:void ls(char *path)
50:    char buf[512], *p;
113:int main(int argc, char *argv[])
huangchangwei@ubuntu:~/svn_rep/linux$
huangchangwei@ubuntu:~/svn_rep/linux$
huangchangwei@ubuntu:~/svn_rep/linux$ grep -nr ^char ls.c
15:char *fmtname(char *path)
huangchangwei@ubuntu:~/svn_rep/linux$
```
grep增加^符号后，输出结果只包含开头为char的对应行信息。

## $ 锚尾，匹配输入行的结尾
^锚定行的结尾，如 ` grep$ ` 匹配所有以 grep 结尾的行。

例如：找出以单词foo结尾的行
```
huangchangwei@ubuntu:~/svn_rep/linux$ grep -nr foo tmp.txt
1:foo
2:foo_1
3:2_foo_3
huangchangwei@ubuntu:~/svn_rep/linux$
huangchangwei@ubuntu:~/svn_rep/linux$ grep -nr foo$ tmp.txt
1:foo
huangchangwei@ubuntu:~/svn_rep/linux$
```
## . 匹配除'\n'外的任何单个字符
例如：`ab.f`等价于`abcf`
## * 匹配前面的字符零次或多次
例如：`zo*`能匹配`z`以及`zoo`。
```
huangchangwei@ubuntu:~/svn_rep/linux$ cat tmp.txt
foo_1
huangchangwei@ubuntu:~/svn_rep/linux$
huangchangwei@ubuntu:~/svn_rep/linux$ grep 'f*_' tmp.txt
foo_1
huangchangwei@ubuntu:~/svn_rep/linux$
huangchangwei@ubuntu:~/svn_rep/linux$ grep 'fo*_' tmp.txt
foo_1
huangchangwei@ubuntu:~/svn_rep/linux$
```
第一次grep，*不匹配f, 按照`_`进行匹配，匹配成功。
第二次grep，按照`foo_`进行匹配，匹配成功。

# gdb
## gdb显示带宏定义的值
编译时增加 `-g3 -gdwarf-2` 参数。
## gdb调试带参数的二进制
` gdb --args ./a.out test_arg `

# vim

## 批量替换文件中的关键字

```
:s/xxx/yyy/g				在当前行将关键字'xxx'替换为关键字'yyy'
:%s/xxx/yyy/g				在所有行将关键字'xxx'替换为关键字'yyy'
```

# sed批量删除前n行或后n行

## 删除前n行

删除tmp.txt中的前两行。

```
sed -i '1,2d' tmp.txt
```

## 删除后n行

删除tmp.txt中的后两行

` sed -i '2,$d' tmp.txt`

# awk

## 删除文件前n列

```
awk '{$1="";print $0'} blu_dtb.txt > new_blu_dtb.txt
```

将第一列置为空，然后将结果输出到新的文件。