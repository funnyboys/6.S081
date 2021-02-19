#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    char *p = 0;
    printf("*p is %p\n", *p);
    exit(0);
}
