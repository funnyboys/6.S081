#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define DIRSIZ 100

char *fmtname(char *path)
{
    static char buf[DIRSIZ+1];
    char *p;

    // Find first character after last slash.
    for(p=path+strlen(path); p >= path && *p != '/'; p--)
        ;
    p++;

    // Return blank-padded name.
    if(strlen(p) >= DIRSIZ)
        return p;
    memmove(buf, p, strlen(p));
    memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
    return buf;
}

void print_type(struct stat st)
{
    switch (st.st_mode & S_IFMT) {
        case S_IFBLK:     printf("block device\n");                 break;
        case S_IFCHR:     printf("character device\n");             break;
        case S_IFDIR:     printf("directory\n");                    break;
        case S_IFIFO:     printf("FIFO/pipe\n");                    break;
        case S_IFLNK:     printf("symlink\n");                      break;
        case S_IFREG:     printf("regular file\n");                 break;
        case S_IFSOCK:    printf("socket\n");                       break;
        default:          printf("unknown?\n");                     break;
    }
}

void ls(char *path)
{
    DIR *dir = NULL;
    char buf[512], *p;
    int fd, len;
    struct dirent *de;
    struct stat st;

    printf("path: %s\n", path);

    if ((fd = open(path, 0)) < 0) {
        printf("ls: cannot open %s\n", path);
        return;
    }

    if (fstat(fd, &st) < 0){
        printf("ls: cannot stat %s\n", path);
        close(fd);
        return;
    }

    //print_type(st);
    switch(st.st_mode & S_IFMT){
        case S_IFREG:
            printf("%s\n", path);
            close(fd);
            break;

        case S_IFDIR:
            close(fd);
            if ((dir = opendir(path)) == NULL) {
                printf("ls: opendir %s fail\n", path);
                return;
            }
            while (1)
            {
                de = readdir(dir);
                if (!de)
                    break;
                len = strlen(path);
                strcpy(buf, path);
                p = buf + len;
                if (buf[len-1] != '/')
                    *p++ = '/';
                memmove(p, de->d_name, 256);
                p[256] = 0;
            #if 0
                if (de->d_type != DT_DIR)
                    printf("de->d_name = %s, de->d_type = 0x%x\n", buf, de->d_type);
                else
                    printf("dir: %s\n", buf);
            #endif
                if (de->d_type == DT_DIR && memcmp(de->d_name, ".", strlen(".")) && memcmp(de->d_name, "..", strlen("..")))
                    ls(buf);
            }

            closedir(dir);
            break;

        default:
            printf("invalid file type!\n");
            close(fd);
            break;
    }
}

int main(int argc, char *argv[])
{
    int i;

    if (argc < 2){
        ls(".");
        exit(0);
    }
    for (i = 1; i < argc; i++)
        ls(argv[i]);
    exit(0);
}
