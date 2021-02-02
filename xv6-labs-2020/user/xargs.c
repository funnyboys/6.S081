#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

#define BUF_SIZE        (256)
int main(int argc, char *argv[])
{
    int len;
    char buf[BUF_SIZE] = {0};

    len = read(0, buf, BUF_SIZE);
    printf("len = %d, str: %s\n", len, buf);

    exit(0);
}
