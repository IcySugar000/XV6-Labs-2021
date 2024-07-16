#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char* argv[]){
    int p[2][2];
    char ch[1];
    pipe(p[0]);  // 父进程写子进程读
    pipe(p[1]);  // 子进程写父进程读

    int pid;
    if(fork() == 0){
        // 子进程
        pid = getpid();
        close(p[0][1]);
        close(p[1][0]);

        read(p[0][0], ch, 1);
        printf("%d: received ping\n", pid);
        close(p[0][0]);

        write(p[1][1], "A", 1);
        close(p[1][1]);

        exit(0);
    }
    else{
        // 父进程
        pid = getpid();
        close(p[0][0]);
        close(p[1][1]);

        write(p[0][1], "M", 1);
        close(p[0][1]);

        read(p[1][0], ch, 1);
        printf("%d: received pong\n", pid);
        close(p[1][0]);

        exit(0);
    }
}