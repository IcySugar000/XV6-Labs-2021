#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void find(char *path, char* target) {
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    fd = open(path, 0);
    fstat(fd, &st);
    close(fd);

    switch(st.type) {
        case T_FILE:
            for(p=path+strlen(path); p >= path && *p != '/'; p--);
            p++;
            if(strcmp(p, target) == 0) {
                printf("%s\n", path);
            }
            break;
        case T_DIR:
            strcpy(buf, path);
            p = buf+strlen(buf);
            *p++ = '/';
            *p = 0;
            
            fd = open(buf, 0);
            while(read(fd, &de, sizeof(de)) == sizeof(de)) {
                if(de.inum == 0)
                    continue;
                if(strcmp(de.name, "..") != 0 && strcmp(de.name, ".") != 0) {
                    memmove(p, de.name, DIRSIZ);
                    p[DIRSIZ] = 0;
                    find(buf, target);
                }
            }
            break;
    }
}

int main(int argc, char* argv[]) {
    if(argc != 3) {
        printf("find: invalid args\n");
        exit(1);
    }

    find(argv[1], argv[2]);

    exit(0);
}