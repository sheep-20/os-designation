#include "kernel/types.h"
#include "user/user.h"

int main() {
  int p1[2], p2[2];  // 两个管道：p1(父->子), p2(子->父)
  char buf[10];

  // 创建两个管道
  pipe(p1);
  pipe(p2);

  if (fork() == 0) {  // 子进程
    close(p1[1]);     // 关闭p1的写端（子进程只读取）
    close(p2[0]);     // 关闭p2的读端（子进程只写入）

    // 从p1读取数据
    read(p1[0], buf, 4);
    printf("%d: received %s\n", getpid(), buf);

    // 向p2写入数据
    write(p2[1], "pong", 4);

    // 关闭剩余文件描述符
    close(p1[0]);
    close(p2[1]);
    exit(0);
  } else {  // 父进程
    close(p1[0]);     // 关闭p1的读端（父进程只写入）
    close(p2[1]);     // 关闭p2的写端（父进程只读取）

    // 向p1写入数据
    write(p1[1], "ping", 4);

    // 等待子进程完成
    wait(0);

    // 从p2读取数据
    read(p2[0], buf, 4);
    printf("%d: received %s\n", getpid(), buf);

    // 关闭剩余文件描述符
    close(p1[1]);
    close(p2[0]);
    exit(0);
  }
}
