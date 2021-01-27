shell下执行各项命令(例如ls)：
1. shell执行fork()
2. 子进程执行execl("ls", ...)替换对应代码
3. 子进程执行完毕返回


文件句柄为0, 为标准输入文件 /dev/stdin
文件句柄为1，为标准输出文件 /dev/stdout
文件句柄为2，为错误输出文件 /dev/stderr
eg:
ls -lh /dev/stdin
    lrwxrwxrwx 1 root root 15 Jan 19 21:17 /dev/stdin -> /proc/self/fd/0
ls -lh /dev/stdout
    lrwxrwxrwx 1 root root 15 Jan 19 21:17 /dev/stdout -> /proc/self/fd/1
ls -lh /dev/stderr
    lrwxrwxrwx 1 root root 15 Jan 19 21:17 /dev/stderr -> /proc/self/fd/2
