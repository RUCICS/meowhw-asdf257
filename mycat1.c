#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "用法: %s <文件名>\n", argv[0]);
        return 1;
    }
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("打开文件失败");
        return 1;
    }
    char c;
    ssize_t n;
    while ((n = read(fd, &c, 1)) == 1) {
        if (write(STDOUT_FILENO, &c, 1) != 1) {
            perror("写入失败");
            close(fd);
            return 1;
        }
    }
    if (n < 0) {
        perror("读取失败");
        close(fd);
        return 1;
    }
    if (close(fd) < 0) {
        perror("关闭文件失败");
        return 1;
    }
    return 0;
} 