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
    int fd = open("output.txt", O_WRONLY | O_CREAT);
    write(fd, "ooo\n", 4);

    exit(0);
}