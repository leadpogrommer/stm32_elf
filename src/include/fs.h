#pragma once


#include <reent.h>

typedef struct {
    // drive letter
    char letter;

    // constructor
    void (*init)();

    // fs methods
    int (*open)(const char *name, int flags, int mode);
    int (*close)(int fd);
    _off_t (*lseek)(int fd, _off_t offset, int whence);
    _ssize_t (*read)(int fd, void *buf, size_t count);
    _ssize_t (*write)(int fd, const void *buf, size_t count);
}fs_driver_t;

void init_fs();