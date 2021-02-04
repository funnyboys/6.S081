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


#define BUF_SIZE        (10)

int main(int argc, char *argv[])
{
    char *p;
    char cmd[BUF_SIZE] = {0};
    char buf[BUF_SIZE] = {0};
    char out[MAXARG][BUF_SIZE] = {0};
    char *args[MAXARG] = {0};
    int i, len, arg_num;
    int right;
    int out_index = 0;

    if (argc < 2)
        exit(0);

    len = read(0, buf, sizeof(buf));
    //printf("[%s %d]: %s\n", __func__, __LINE__, buf);

#if 0
    len = strlen("/bin/");
    strcpy(cmd, "/bin/");
    p = cmd + len;
#endif
    (void)p;
    strcpy(cmd, argv[1]);
    //printf("[%s %d]: %s\n", __func__, __LINE__, cmd);

    arg_num = 0;
    args[arg_num++] = cmd;
    for (i = 2; i < argc; i++)
        args[arg_num++] = argv[i];

    //parse_str("hello too", strlen("hello too"));
    len = strlen(buf);
    p = buf;
    while (1)
    {
        right = parse_args(p, len, out[out_index]);
        if (right == -1)
            break;
        //printf("[%s %d]: %s\n", __func__, __LINE__, out[out_index]);
        args[arg_num++] = out[out_index++];

        //printf("[%s %d]: %d %d\n", __func__, __LINE__, right, len);
        p += right;
        len -= right;
        //printf("[%s %d]: %d %d\n", __func__, __LINE__, right, len);
    }

#if 0
    for (i = 0; i < arg_num; i++)
        printf("[%s %d]: arg[%d] = %s\n", __func__, __LINE__, i, args[i]);
#endif

    exec(cmd, args);

#if 0
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
        if(de.inum == 0)
            continue;
        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;
        if(stat(buf, &st) < 0){
            printf("ls: cannot stat %s\n", buf);
            continue;
        }
        printf("%s %d %d %d\n", fmtname(buf), st.type, st.ino, st.size);
    }
#endif

    exit(0);
}
