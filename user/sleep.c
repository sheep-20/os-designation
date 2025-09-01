#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
  // 检查参数数量：必须提供一个数字作为睡眠的滴答数
  if (argc != 2) {
    fprintf(2, "用法: sleep <ticks>\n");  // 2 表示标准错误输出
    exit(1);  // 退出并返回错误码
  }

  // 将字符串参数转换为整数（滴答数）
  int ticks = atoi(argv[1]);

  // 调用 sleep 系统调用
  sleep(ticks);

  // 成功执行后退出
  exit(0);
}
