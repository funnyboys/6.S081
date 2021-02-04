#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "kernel/param.h"
#include "user/user.h"

static int parse_args(char *str, int str_len, char *out)
{
    int i, len;
    int left = -1, right = -1;

    for (i = 0; i < str_len; i++)
        if (str[i] != ' ' && str[i] != '\n' && str[i] != '\r' && str[i] != '\0') {
            left = i;
            break;
        }

    if (left == -1)
        return -1;

    for (i = left + 1; i < str_len; i++)
        if (str[i] == ' ' || str[i] == '\n' || str[i] == '\r' || str[i] == '\0') {
            right = i;
            break;
        }

    if (right == -1)
        return -1;

    len = right - left;
    memcpy(out, str + left, len);
    out[len] = '\0';

    return right;
}

/* 读取字符串, 换行结束 */
static int getcmd(char *buf, int nbuf)
{
    memset(buf, 0, nbuf);
    gets(buf, nbuf);
    if (buf[0] == 0) // EOF
        return -1;
    return 0;
}

#define BUF_SIZE        (50)

int main(int argc, char *argv[])
{
    char *p;
    char buf[BUF_SIZE] = {0};
    char tmp[MAXARG][BUF_SIZE] = {0};
    char *args[MAXARG] = {0};
    int i, right, len;
    int fix_narg = 0, tmp_index = 0;

    if (argc < 2)
        exit(0);

    args[fix_narg++] = argv[1];
    for (i = 2; i < argc; i++)
        args[fix_narg++] = argv[i];

    while (getcmd(buf, BUF_SIZE) != -1)
    {
        i = fix_narg;
        len = strlen(buf);
        p = buf;
        while (1)
        {
            right = parse_args(p, len, tmp[tmp_index]);
            if (right == -1)
                break;
            args[i++] = tmp[tmp_index++];
            p += right;
            len -= right;
        }

#if 0
        for (i = 0; i < index; i++)
            printf("[%s %d]: arg[%d] = %s\n", __func__, __LINE__, i, args[i]);
#endif

        if (fork() == 0)
            exec(args[0], args);
        else {
            wait(0);
            tmp_index = 0;
            memset(args + fix_narg, 0, sizeof(char *) * (MAXARG - fix_narg));
        }

    }
    exit(0);
}
