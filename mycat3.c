#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>

// 获取缓冲区大小（系统内存页大小）
size_t io_blocksize() {
    long sz = sysconf(_SC_PAGESIZE);
    if (sz <= 0) {
        // 获取失败时默认4K
        return 4096;
    }
    return (size_t)sz;
}

// 分配对齐到内存页的缓冲区
char* align_alloc(size_t size) {
    void* ptr = NULL;
    size_t alignment = io_blocksize();
    if (posix_memalign(&ptr, alignment, size) != 0) {
        return NULL;
    }
    return (char*)ptr;
}

// 释放对齐分配的缓冲区
void align_free(void* ptr) {
    free(ptr);
}

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
    size_t bufsize = io_blocksize();
    char *buf = align_alloc(bufsize);
    if (!buf) {
        perror("分配缓冲区失败");
        close(fd);
        return 1;
    }
    ssize_t n;
    while ((n = read(fd, buf, bufsize)) > 0) {
        ssize_t written = 0;
        while (written < n) {
            ssize_t w = write(STDOUT_FILENO, buf + written, n - written);
            if (w < 0) {
                perror("写入失败");
                align_free(buf);
                close(fd);
                return 1;
            }
            written += w;
        }
    }
    if (n < 0) {
        perror("读取失败");
        align_free(buf);
        close(fd);
        return 1;
    }
    align_free(buf);
    if (close(fd) < 0) {
        perror("关闭文件失败");
        return 1;
    }
    return 0;
} 