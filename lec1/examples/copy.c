#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main()
{
    char buf[64];

    while(1) {
        /* 
         * ssize_t read(int fd, void *buf, size_t count); 
         *
         * 文件句柄为0, 为标准输入文件 /dev/stdin
         * 文件句柄为1，为标准输出文件 /dev/stdout
         * 文件句柄为2，为错误输出文件 /dev/stderr
         * 
         * ls -lh /dev/stdin
         *    lrwxrwxrwx 1 root root 15 Jan 19 21:17 /dev/stdin -> /proc/self/fd/0
         * ls -lh /dev/stdout
         *    lrwxrwxrwx 1 root root 15 Jan 19 21:17 /dev/stdout -> /proc/self/fd/1
         * ls -lh /dev/stderr
         *    lrwxrwxrwx 1 root root 15 Jan 19 21:17 /dev/stderr -> /proc/self/fd/2
         *
         */
        int n = read(0, buf, sizeof(buf));
        if(n <= 0)
            break;
        write(1, buf, n);
    }

    exit(0);
}
