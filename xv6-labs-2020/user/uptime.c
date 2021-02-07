#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "kernel/param.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    printf("up %d\n", uptime());
    exit(0);
}
