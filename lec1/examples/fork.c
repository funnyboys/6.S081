#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// open.c: create a file, write to it.
int main()
{
    int pid;

    pid = fork();
    printf("fork() returned %d\n", pid);

    /* fork() = 0, 当前是子进程 */
    if(pid == 0){
        printf("child\n");
    } else {    /* fork() != 0，当前是父进程，返回子进程的pid */
        printf("parent\n");
    }

    exit(0);
}