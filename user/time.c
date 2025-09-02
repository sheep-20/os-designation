#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

int main(int argc, char *argv[])
{
  if (argc < 2) {
    fprintf(2, "Usage: time <command>\n");
    exit(1);
  }

  int start = uptime();

  int pid = fork();
  if (pid < 0) {
    fprintf(2, "fork failed\n");
    exit(1);
  } else if (pid == 0) {
    exec(argv[1], argv + 1);
    fprintf(2, "exec %s failed\n", argv[1]);
    exit(1);
  } else {
    wait(0);
    int end = uptime();
    int delta = end - start;

    // convert delta to string
    char buf[16];
    int i = 0;
    if (delta == 0) {
      buf[i++] = '0';
    } else {
      char tmp[16];
      int j = 0;
      while (delta > 0) {
        tmp[j++] = '0' + (delta % 10);
        delta /= 10;
      }
      while (j > 0) {
        buf[i++] = tmp[--j];
      }
    }
    buf[i++] = '\n';

    // write to time.txt
    int fd = open("time.txt", O_CREATE | O_WRONLY);
    if (fd < 0) {
      fprintf(2, "time: cannot open time.txt\n");
      exit(1);
    }
    write(fd, buf, i);
    close(fd);

    // super long delay to flush fs.img
    sleep(100);  // 100 ticks ~1s in xv6 time
  }

  exit(0);
}

