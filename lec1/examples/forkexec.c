#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// open.c: create a file, write to it.
int main()
{
    int pid, status;

    /* 
     * int execl(const char *path, const char *arg, ...
     * The exec() family  of  functions  replaces  the current process image with a new process image.
     * 
     * exec()会使用新程序，替代当前运行的进程上下文，参数通过argv进行传递
     */

    pid = fork();
    if(pid == 0){
        execl("/bin/ls", "ls", "-al", "/etc/passwd", NULL);
        printf("exec failed!\n");   /* exec not return */
        exit(1);
    } else {
        printf("parent waiting\n");
        /* The wait() system call suspends execution of the calling process until one of its children terminates. */
        wait(&status);
        printf("the child exited with status %d\n", status);
    }

    exit(0);
}

/*
 *  yellow@ubuntu:~/6.S081/lec1/examples$ ./a.out
 *  parent waiting
 *  -rw-r--r-- 1 root root 2556 Apr 26  2020 /etc/passwd
 *  the child exited with status 0 
 */