#include <fs.h>
#include <reent.h>
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <usb_fs.h>

// fs api
// -----------------
// close
// fstat
// isatty (stub is enough)
// lseek
// open`
// read
// stat
// unlink
// write

typedef struct {
    char fs;
    int internal_fd;
}internal_fd_t;

#define FD_COUNT 10
internal_fd_t fds[FD_COUNT];

#define FS_COUNT 1
#define STD_IO_FS 0
static fs_driver_t *fs_drivers[FS_COUNT];


extern int _isatty_r (struct _reent *reent, int fd){
    if(fd >= 0 && fd < 3)return 1;
    return 0;
}

// STUB
int _fstat_r(struct _reent *reent, int fd, struct stat *stat){
    stat->st_mode = S_IFCHR;
    return 0;
}

int _open_r (struct _reent *reent, const char *name, int flags, int mode){
    if(name[0] != '/'){
        reent->_errno = ENOENT;
        return -1;
    }
    int fs = -1;
    for(int i = 0; i < FS_COUNT; i++){
        if(fs_drivers[i]->letter == name[1]){
            fs = i;
            break;
        }
    }
    if(fs == -1){
        reent->_errno = ENOENT;
        return -1;
    }
    int fd = -1;
    for(int i = 0; i < FD_COUNT; i++){
        if(fds[i].internal_fd == -1){
            fd = i;
            break;
        }
    }
    if(fd == -1){
        reent->_errno = ENOMEM;
    }
    int internal_fd = fs_drivers[fs]->open(name+2, flags, mode);
    if(internal_fd < 0){
        return internal_fd;
    }
    fds[fd].fs = fs;
    fds[fd].internal_fd = internal_fd;
    return fd;
}

int _close_r(struct _reent * reent, int fd){
    int ret = fs_drivers[fds[fd].fs]->close(fds[fd].internal_fd);
    if (ret == 0){
        fds[fd].internal_fd = -1;
        fds[fd].fs = -1;
    }
    return ret;
}

_off_t _lseek_r (struct _reent *reent, int fd, _off_t offset, int whence){
    return fs_drivers[fds[fd].fs]->lseek(fds[fd].internal_fd, offset, whence);
}

_ssize_t _read_r (struct _reent *reent, int fd, void *buf, size_t count){
    return fs_drivers[fds[fd].fs]->read(fds[fd].internal_fd, buf, count);
}

_ssize_t _write_r (struct _reent *reent, int fd, const void *buf, size_t count){
    return fs_drivers[fds[fd].fs]->write(fds[fd].internal_fd, buf, count);
}


void init_fs(){
    fs_drivers[0] = &usb_hostfs_driver;


    for(int i = 0; i < FS_COUNT; i++){
        if(fs_drivers[i]->init)fs_drivers[i]->init();
    }
    for(int i = 0; i < FD_COUNT; i++){
        fds[i].fs = -1;
        fds[i].internal_fd = -1;
    }
    // open stdin, stderr, stdout
    for(int i = 0; i < 3; i++){
        fds[i].fs = STD_IO_FS;
        fds[i].internal_fd = i;
    }

}
