#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    // 参数错误
    if(argc != 2) {
        printf("usage: sleep [int time]");
        exit(1);
    }

    sleep(atoi(argv[1]));
    exit(0);
}