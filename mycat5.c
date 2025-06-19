#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <string.h>

// 获取大于等于x的最小2的幂
size_t next_pow2(size_t x) {
    if (x == 0) return 1;
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
#if ULONG_MAX > 0xffffffff
    x |= x >> 32;
#endif
    return x + 1;
}

// 通过实验，假设A=8
#define BUF_MULTIPLIER 8

// 获取缓冲区大小（文件系统块大小的8倍，且为2的幂）
size_t io_blocksize(int fd) {
    long page_sz = sysconf(_SC_PAGESIZE);
    size_t page_size = (page_sz > 0) ? (size_t)page_sz : 4096;
    struct stat st;
    size_t block_size = 0;
    if (fstat(fd, &st) == 0) {
        block_size = st.st_blksize;
    }
    if (block_size < 512 || (block_size & (block_size - 1)) != 0) {
        block_size = page_size;
    }
    size_t bufsize = block_size * BUF_MULTIPLIER;
    return next_pow2(bufsize);
}

// 分配对齐到内存页的缓冲区
char* align_alloc(size_t size) {
    void* ptr = NULL;
    size_t alignment = sysconf(_SC_PAGESIZE);
    if (alignment <= 0) alignment = 4096;
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
    size_t bufsize = io_blocksize(fd);
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