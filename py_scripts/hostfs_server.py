import usb.core
import array
import time
import sys
import os
import struct

control_ep = 0x81
data_out_ep = 0x02
data_in_ep = 0x82

FCNTL = {
    0: os.O_RDONLY,
    1: os.O_WRONLY,
    2: os.O_RDWR,
    0x0008: os.O_APPEND,
    0x0200: os.O_CREAT,
    0x0400: os.O_TRUNC,
    0x0800: os.O_EXCL,
    0x2000: os.O_SYNC,
}

enable_log = False

def log(*args, **kwargs):
    if enable_log:
        print('>>>', *args, **kwargs, file=sys.stderr)

def handle_write(command: bytes, data: bytes)-> list[bytes]:
    fd = struct.unpack('<bi', command)[1]
    res = os.write(fd, data)
    return [res.to_bytes(4, 'little', signed=False)]

def handle_open(command: bytes, data: bytes)->list[bytes]:
    cmd = struct.unpack('<bii32s', command)
    flags = cmd[1]
    mode = cmd[2]
    path = cmd[3].split(b'\0', 1)[0].decode()
    if path[0] == '/':
        path = '/tmp' + path
    log(f'open: {flags=}, {mode=}, {path=}')

    host_flags = 0
    for i in range(32):
        if (1 << i) & flags:
            host_flags |= FCNTL[1 << i]
    
    log(f'open: host_flags: {hex(host_flags)}')

    res = os.open(path, host_flags, mode)
    return[res.to_bytes(4, 'little', signed=False)]

def handle_close(command: bytes, data: bytes)->list[bytes]:
    _, fd = struct.unpack('<bi', command)
    os.close(fd)
    res = 0
    return[res.to_bytes(4, 'little', signed=False)]

def handle_lseek(command: bytes, data: bytes)->list[bytes]:
    _, fd, offset, whence = struct.unpack('<biii', command)
    res = os.lseek(fd, offset, whence)
    return[res.to_bytes(4, 'little', signed=True)]

def handle_read(command: bytes, data: bytes)->list[bytes]:
    _, fd, count = struct.unpack('<biI', command)
    data = os.read(fd, count)
    return [len(data).to_bytes(4, 'little', signed=True), data]

request_handlers = {
    1: handle_write,
    2: handle_open,
    3: handle_close,
    4: handle_lseek,
    5: handle_read,
}


if __name__ == '__main__':
    dev: usb.core.Device = usb.core.find(idVendor=0xCAFE, idProduct = 0xBABE)
    dev.reset()
    dev.set_configuration()

    dev.ctrl_transfer(0x40, 0, 0, 0, 'h');

    b1 = bytes([0x13,0x37])
    while True:
        cmd = dev.read(control_ep, 64, timeout=1000000).tobytes()
        log(f'Got command: {cmd}')

        data = bytearray()
        while True:
            packet = dev.read(data_in_ep, 64, timeout=1000000).tobytes()
            log(packet)
            if(len(packet)) == 0:
                break
            data += packet
        log(f'data: {data}')
        cmd_num = struct.unpack('<b', cmd[:1])[0]
        res = request_handlers[cmd_num](cmd, data)
        for buffer in res:
            pos = 0
            l = len(buffer)
            while pos < l:
                send_len = min(64, l - pos)
                log(f'send_len: {send_len}')
                dev.write(data_out_ep, buffer[pos:pos+send_len], timeout=1000000)
                pos += send_len
            dev.write(data_out_ep, bytes([]))
        log('done')



