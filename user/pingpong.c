#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main() {
    int p1[2], p2[2];  // 两个管道：p1(父→子), p2(子→父)
    char buf;

    // 创建管道
    if (pipe(p1) < 0 || pipe(p2) < 0) {
        fprintf(2, "pipe failed\n");
        exit(1);
    }

    int pid = fork();
    if (pid < 0) {
        fprintf(2, "fork failed\n");
        exit(1);
    }

    if (pid == 0) {  // 子进程
        // 关闭不需要的管道端
        close(p1[1]);  // 关闭p1的写端（子进程只需要读）
        close(p2[0]);  // 关闭p2的读端（子进程只需要写）

        // 从父进程读数据
        if (read(p1[0], &buf, 1) != 1) {
            fprintf(2, "child read failed\n");
            exit(1);
        }
        printf("%d: received ping\n", getpid());  // 输出子进程PID

        // 向父进程写数据
        if (write(p2[1], &buf, 1) != 1) {
            fprintf(2, "child write failed\n");
            exit(1);
        }

        // 关闭管道
        close(p1[0]);
        close(p2[1]);
        exit(0);
    } else {  // 父进程
        // 关闭不需要的管道端
        close(p1[0]);  // 关闭p1的读端（父进程只需要写）
        close(p2[1]);  // 关闭p2的写端（父进程只需要读）

        // 向子进程写数据
        if (write(p1[1], "x", 1) != 1) {
            fprintf(2, "parent write failed\n");
            exit(1);
        }

        // 等待子进程完成
        wait(0);

        // 从子进程读数据
        if (read(p2[0], &buf, 1) != 1) {
            fprintf(2, "parent read failed\n");
            exit(1);
        }
        printf("%d: received pong\n", getpid());  // 输出父进程PID

        // 关闭管道
        close(p1[1]);
        close(p2[0]);
        exit(0);
    }
}
