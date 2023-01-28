#include <usb_fs.h>
#include <fs.h>
#include <stdint.h>
#include <string.h>
#include "usb_request.h"
#include <fcntl.h>
#include <unistd.h>

#define VAR_BUFFER(var, buffer) request_buffer_t buffer = {&var, sizeof(var)}
#define NAME_LENGTH 32

static uint8_t command_buffer[64];


#define CMD_OPEN 2
#define CMD_CLOSE 3
#define CMD_LSEEK 4
#define CMD_READ 5
#define CMD_WRITE 1

typedef struct {
    char cmd;
    int32_t flags;
    int32_t mode;
    char path[NAME_LENGTH];
}__attribute__((packed)) open_command_t;

typedef struct {
    char cmd;
    uint32_t fd;
}__attribute__((packed)) close_command_t;

typedef struct {
    char cmd;
    int fd;
    _off_t offset;
    int whence;
}__attribute__((packed)) lseek_command_t;

typedef struct {
    char cmd;
    int fd;
    size_t count;
}__attribute__((packed)) read_command_t;

typedef struct {
    char cmd;
    uint32_t fd;
}__attribute__((packed)) write_command_t;


// RETURN: B0: length_written;
static _ssize_t host_write(int fd, const void *buf, size_t count){
    write_command_t *cmd = (write_command_t*) command_buffer;
    cmd->cmd = CMD_WRITE;
    cmd->fd = fd;

    _ssize_t res;
    request_buffer_t send_buffer = {(void *)buf, count};
    request_buffer_t result_buffer = {&res, sizeof(res)};

    usb_make_request(cmd, sizeof(write_command_t), &send_buffer, &result_buffer, 1);
    return res;
}

// RETURN: B0: fd
static int host_open(const char* path, int flags, int mode){
    open_command_t *cmd = (open_command_t*) command_buffer;
    cmd->cmd = CMD_OPEN;
    cmd->mode = mode;
    cmd->flags = flags;
    strncpy(cmd->path, path, NAME_LENGTH);

    int res;
    request_buffer_t result_buffer = {&res, sizeof(res)};

    usb_make_request(cmd, sizeof(open_command_t), 0, &result_buffer, 1);
    return res;
}

static _off_t host_lseek(int fd, _off_t offset, int whence){
    lseek_command_t *cmd = (lseek_command_t*) command_buffer;
    cmd->cmd = CMD_LSEEK;
    cmd->fd = fd;
    cmd->offset = offset;
    cmd->whence = whence;

    _off_t res;
    VAR_BUFFER(res, result_buffer);

    usb_make_request(cmd, sizeof(lseek_command_t), 0, &result_buffer, 1);
    return res;
}

static int host_close(int fd){
    close_command_t *cmd = (close_command_t *)command_buffer;
    cmd->cmd = CMD_CLOSE;
    cmd->fd = fd;

    int res;
    VAR_BUFFER(res, result_buffer);

    usb_make_request(cmd, sizeof(close_command_t), 0, &result_buffer, 1);
    return res;
}

static _ssize_t host_read(int fd, void *buf, size_t count){
    read_command_t *cmd = (read_command_t *)command_buffer;
    cmd->cmd = CMD_READ;
    cmd->fd = fd;
    cmd->count = count;

    _ssize_t res;
    request_buffer_t result_buffers[2] = {
            {&res, sizeof(res)},{buf, count}
    };

    usb_make_request(cmd, sizeof(read_command_t), 0, result_buffers, 2);
    return res;
}



fs_driver_t usb_hostfs_driver={
        .letter = 'h',

        .open = host_open,
        .close = host_close,
        .lseek = host_lseek,
        .read = host_read,
        .write = host_write,
};