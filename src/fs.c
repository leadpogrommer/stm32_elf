#include <fs.h>
#include <reent.h>
#include <stdio.h>
#include <sys/stat.h>

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

// stub
int _close_r(struct _reent * reent, int fd){
    return -1;
}

//stub
int _fstat_r(struct _reent *reent, int fd, struct stat *stat){
    stat->st_mode = S_IFCHR;
    return 0;
}

extern int _isatty_r (struct _reent *reent, int fd){
    if(fd >= 0 && fd < 3)return 1;
    return 0;
}

// lseek, open, read

