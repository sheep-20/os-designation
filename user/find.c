#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

// 匹配文件名和目标名称
int match(char *path, char *name) {
    char *p;

    // 找到路径中最后一个'/'的位置，提取文件名部分
    for (p = path + strlen(path); p >= path && *p != '/'; p--);
    p++;  // 指向文件名的第一个字符

    // 比较文件名和目标名称
    return strcmp(p, name) == 0;
}

// 递归查找文件
void find(char *path, char *name) {
    int fd;
    struct dirent de;
    struct stat st;
    char buf[512];  // 用于构建子路径
    char *p;

    // 尝试打开路径
    if ((fd = open(path, 0)) < 0) {
        fprintf(2, "find: 无法打开 %s\n", path);
        return;
    }

    // 获取文件状态信息
    if (fstat(fd, &st) < 0) {
        fprintf(2, "find: 无法获取 %s 的状态\n", path);
        close(fd);
        return;
    }

    switch (st.type) {
        case T_FILE:
            // 如果是文件，检查是否匹配目标名称
            if (match(path, name)) {
                printf("%s\n", path);
            }
            break;

        case T_DIR:
            // 检查路径长度是否超过缓冲区限制
            if (strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)) {
                printf("find: 路径 %s 太长\n", path);
                break;
            }

            // 构建基础路径
            strcpy(buf, path);
            p = buf + strlen(buf);
            *p++ = '/';  // 添加路径分隔符

            // 遍历目录中的所有条目
            while (read(fd, &de, sizeof(de)) == sizeof(de)) {
                // 跳过空条目
                if (de.inum == 0) {
                    continue;
                }
                
                // 跳过当前目录 (.) 和上级目录 (..)
                if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) {
                    continue;
                }

                // 将目录项名称复制到路径缓冲区
                memmove(p, de.name, DIRSIZ);
                p[DIRSIZ] = 0;  // 先确保有终止符
                
                // 找到实际文件名的结尾并正确终止字符串
                char *end = p;
                while (*end && end < p + DIRSIZ) {
                    end++;
                }
                *end = 0;

                // 递归查找子目录
                find(buf, name);
            }
            break;
    }

    close(fd);
}

int main(int argc, char *argv[]) {
    // 检查命令行参数是否正确
    if (argc != 3) {
        fprintf(2, "用法: find <路径> <文件名>\n");
        exit(1);
    }

    // 开始查找
    find(argv[1], argv[2]);
    exit(0);
}

