#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void prime_sieve(int pip[]) {
    // 递归函数创建子进程
    int buf[1];
    int cur_num;
    close(pip[1]);

    // 递归终止条件：
    // 如果read为空，即管道内所有东西都被读完，此时所有数都被筛完
    if(read(pip[0], buf, sizeof(int)) == 0) {
        close(pip[0]);
        return;
    }
    else {
        cur_num = buf[0];
        printf("prime %d\n", cur_num);
    }

    int new_p[2];
    pipe(new_p);
    if(fork() == 0) {
        // 在子进程进行递归
        prime_sieve(new_p);
        wait((int *) 0 );  // 每一层都要等待！
        exit(0);
    }
    else {
        // 在父进程筛素数，并传给管道
        close(new_p[0]);
        while(read(pip[0], buf, sizeof(int)) != 0){
            if(buf[0] % cur_num != 0){
                write(new_p[1], buf, sizeof(int));
            }
        }
        close(pip[0]);
        close(new_p[1]);
    }        
    return;
}

int main(int argc, char* argv[]) {
    int p[2];
    pipe(p);

    int buf[1];

    if(fork() == 0){
        prime_sieve(p);
        wait((int *) 0 );
        exit(0);
    }
    else {
        close(p[0]);
        for(int i = 2; i <= 35; ++i){
            buf[0] = i;
            write(p[1], buf, sizeof(int));
        }
        close(p[1]);
        wait((int *) 0 );
        exit(0);
    }
}