#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void find(char *path, char *name)
{
    int fd, len;
    char buf[512], *p;
    struct dirent de;
    struct stat st;

    if ((fd = open(path, 0)) < 0){
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if (fstat(fd, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    if (strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)) {
        printf("find: path too long!\n");
        close(fd);
        return;
    }

    len = strlen(path);
    strcpy(buf, path);
    p = buf + len;
    if (buf[len-1] != '/')
        *p++ = '/';

    while (read(fd, &de, sizeof(de)) == sizeof(de))
    {
        if (de.inum == 0)
            continue;
        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;
        if (stat(buf, &st) < 0){
            printf("find: cannot stat %s\n", buf);
            continue;
        }

        switch (st.type) {
        case T_DIR:
            if (memcmp(de.name, ".", strlen(".")) && memcmp(de.name, "..", strlen("..")))
                find(buf, name);
            break;

        case T_DEVICE:
        case T_FILE:
            if (memcmp(de.name, name, strlen(name)) == 0)
                printf("%s\n", buf);
            break;

        default:
            printf("find: unsupport, st.type = %u\n", st.type);
            break;
        }
    }

    close(fd);
}

int main(int argc, char *argv[])
{
    char *cur_dir = "./";
    char *path, *name;

    if (argc != 2 && argc != 3) {
        printf("input arg error!\n");
        exit(1);
    }
    if (argc == 2) {
        path = cur_dir;
        name = argv[1];
    } else {
        path = argv[1];
        name = argv[2];
    }

    find(path, name);
    exit(0);
}
