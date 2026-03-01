// Copyright (C) 2026 Piers Finlayson <piers@piers.rocks>
//
// MIT License

//
// Implementations of libc functions used by the examples.  Enables the
// examples to be built without linking against newlib/libc.
//

#include <stdint.h>
#include <stddef.h>
#include <sys/stat.h>
#include "pico.h"

void __assert_func(const char *file, int line, const char *func, const char *expr) {
    (void)file; (void)line; (void)func; (void)expr;
    __asm volatile ("bkpt #0");
    for(;;);
}

void *memcpy(void *dst, const void *src, size_t n) {
    uint8_t *d = dst;
    const uint8_t *s = src;
    while (n--) *d++ = *s++;
    return dst;
}

void *memset(void *dst, int c, size_t n) {
    uint8_t *d = dst;
    while (n--) *d++ = (uint8_t)c;
    return dst;
}

size_t strlen(const char *s) {
    size_t n = 0;
    while (*s++) n++;
    return n;
}

void _exit(int status) { (void)status; while(1); }
int _close(int fd) { (void)fd; return -1; }
int _fstat(int fd, struct stat *st) { (void)fd; (void)st; return -1; }
int _isatty(int fd) { (void)fd; return 1; }
int _lseek(int fd, int offset, int whence) { (void)fd; (void)offset; (void)whence; return -1; }
int _read(int fd, char *buf, int len) { (void)fd; (void)buf; (void)len; return -1; }
int _write(int fd, char *buf, int len) { (void)fd; (void)buf; (void)len; return -1; }
int _sbrk(int incr) { (void)incr; return -1; }
int _kill(int pid, int sig) { (void)pid; (void)sig; return -1; }
int _getpid(void) { return 1; }