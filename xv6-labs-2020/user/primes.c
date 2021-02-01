/*
 * 参考
 * https://blog.csdn.net/RedemptionC/article/details/106484363#commentBox
 * 
 */
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define MAX_PRIME       (35)

int main(void)
{
    int fd[2], left_data[100];
    int i, cnt, pid, first, current, prime;

    cnt = 0;
    for (i = 2; i <= MAX_PRIME; i++)
        left_data[cnt++] = i;

    while (cnt > 0)
    {
        pipe(fd);
        pid = fork();
        if (pid == 0) {
            close(fd[1]);
            first = 1;
            while (read(fd[0], &current, sizeof(current)) != 0) {
                //printf("current: %d, first = %d\n", current, first);
                if (first == 1) {
                    prime = current;
                    printf("prime %d\n", prime);
                    cnt = 0;
                    first = 0;
                } else {
                    if (current % prime)
                        left_data[cnt++] = current;
                }
            }
            //printf("cnt: %d\n", cnt);
            close(fd[0]);
        } else if (pid > 0) {
            close(fd[0]);
            for (i = 0; i < cnt; i++)
                write(fd[1], &left_data[i], sizeof(left_data[i]));
            close(fd[1]);
            wait(0);
            break;
        } else {
            printf("fork() error!\n");
            close(fd[0]);
            close(fd[1]);
            exit(1);;
        }
    }

    exit(0);
}