#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// redirect.c: run a command with output redirected
int main()
{
    int pid;

    pid = fork();
    if (pid == 0) {

        /* 
         * fd = 1，代表标准输出
         * open()返回的当前最小的文件句柄号，这里返回1
         * 所以之后所有的 echo 内容都会写入到 output.txt 文件中
         * */
        close(1);
        open("output.txt", O_WRONLY | O_CREAT);

        const char *argv[] = { "echo", "this", "is", "redirected", "echo", NULL};
        execl("/bin/echo", "echo", "this", "is", "redirected", "echo", NULL);
        printf("exec failed!\n");
        exit(1);
    } else {
        wait((int *) 0);
    }

    exit(0);
}

/*
 *  yellow@ubuntu:~/6.S081/lec1/examples$ ./a.out
 *  yellow@ubuntu:~/6.S081/lec1/examples$
 *  yellow@ubuntu:~/6.S081/lec1/examples$ cat output.txt
 *  this is redirected echo
 * */