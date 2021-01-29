#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

/*
 * Write a program that uses UNIX system calls to ''ping-pong'' a byte 
 * between two processes over a pair of pipes, one for each direction. 
 * The parent should send a byte to the child; 
 * the child should print "<pid>: received ping", where <pid> is its process ID,
 * write the byte on the pipe to the parent, and exit; 
 * the parent should read the byte from the child, print "<pid>: received pong", and exit. 
 * Your solution should be in the file user/pingpong.c.
 */
int main(int argc, char *argv[])
{
    char data[64];
    int parent_pipe[2], child_pipe[2];

    pipe(parent_pipe);
    pipe(child_pipe);

    if (fork()) {   /* parent */
        close(parent_pipe[0]);
        close(child_pipe[1]);
        write(parent_pipe[1], "ping", strlen("ping"));
        read(child_pipe[0], data, 4);
        printf("%d: received %s\n", getpid(), data);
        close(parent_pipe[1]);
        close(child_pipe[0]);

    } else {         /* child */
        close(parent_pipe[1]);
        close(child_pipe[0]);
        read(parent_pipe[0], data, 4);
        printf("%d: received %s\n", getpid(), data);
        write(child_pipe[1], "pong", strlen("pong"));
        close(parent_pipe[0]);
        close(child_pipe[1]);
    }

    exit(0);
}
