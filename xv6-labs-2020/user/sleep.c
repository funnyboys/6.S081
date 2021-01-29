#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

/*
 * Implement the UNIX program sleep for xv6; 
 * your sleep should pause for a user-specified number of ticks. 
 * A tick is a notion of time defined by the xv6 kernel, namely the time between two interrupts from the timer chip. 
 * Your solution should be in the file user/sleep.c.
 */
int main(int argc, char *argv[])
{
    int time;

    if (argc != 2)
        goto error;

    time = atoi(argv[1]);
    if (time == 0)
        goto error;

    if (sleep(time))
        goto error;

    exit(0);

error:
    printf("input arg error!\n");
    exit(1);
}