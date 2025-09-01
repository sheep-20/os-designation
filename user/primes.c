#include "kernel/types.h"
#include "user/user.h"

#define INDEX_READ 0
#define INDEX_WRITE 1

//创建函数是因为会反复调用这个函数（递归）
void child(int fds_p2c[])
{
    //通讯方向：父进程到子进程，子进程不需要写
    close(fds_p2c[INDEX_WRITE]);

    int i;
    if (read(fds_p2c[INDEX_READ], &i, sizeof(i)) == 0)
    {
        //如果已经读不到数字了，说明已经全部素数都输出完毕了。这个时候子进程可以关闭管道，直接退出。
        close(fds_p2c[INDEX_READ]);
        exit(0);
    }
    printf("prime %d\n", i);
    int num = 0;
    //子进程到孙子进程的管道
    int fds_c2gc[2];
    pipe(fds_c2gc);
    int pid;
    //孙子进程,递归调用本函数
    if ((pid = fork()) == 0)
    {

        child(fds_c2gc);
    }
    //子进程
    else
    {
        //通讯方向：子进程到孙子进程，子进程不需要读
        close(fds_c2gc[INDEX_READ]);
        while (read(fds_p2c[INDEX_READ], &num, sizeof(num)) > 0)
        {
            //如果可以不整除才发出去
            if (num % i != 0)
            {
                write(fds_c2gc[INDEX_WRITE], &num, sizeof(num));
            }
        }
        close(fds_c2gc[INDEX_WRITE]);
        //一样要等待所有的子进程结束
        wait(0);
    }

    //子进程结束
    exit(0);
}

int main(int argc, char *argv[])
{
    int fds_p2c[2]; //父进程到子进程
    pipe(fds_p2c);
    int pid;
    //子进程
    if ((pid = fork()) == 0)
    {
        child(fds_p2c);
    }
    //父进程
    else
    {
        //通讯方向：父进程到子进程，子进程不需要读
        close(fds_p2c[INDEX_READ]);
        for (int i = 2; i <= 35; i++)
        {
            write(fds_p2c[INDEX_WRITE], &i, sizeof(i));
        }

        //必须关闭管道，否则子进程在read函数阻塞
        close(fds_p2c[INDEX_WRITE]);

        //等待子进程结束
        wait(0);
    }

    //父进程结束
    exit(0);
}

