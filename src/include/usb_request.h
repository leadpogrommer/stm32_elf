#pragma once

#include <libopencm3/usb/usbd.h>

usbd_device* usb_setup(void);
int is_initialized();

typedef struct{
    void *buff;
    size_t len;
    size_t pos;
}request_buffer_t;

void usb_make_request(void *command, size_t command_size, request_buffer_t* send_buffer, request_buffer_t* receive_buffers, int request_buffer_count);