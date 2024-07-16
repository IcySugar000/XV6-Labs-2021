#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int main(int argc, char* argv[]) {
    char buf[512];
    int i=0;
    while(read(0, buf + i, sizeof(char)) != 0) {
        if(buf[i] == '\n'){
            buf[i] = 0;
            if(fork() == 0) {
                char* new_argv[MAXARG];
                new_argv[0] = argv[1];
                new_argv[1] = argv[2];
                new_argv[2] = buf;
                exec(argv[1], new_argv);
                exit(0);
            }
            else {
                wait(0);
            }
            i = 0;
        }
        else {
            ++i;
        }
    }
    exit(0);
}