progs = hello.elf calc.elf some_tests.elf

all: $(progs)

CC=arm-none-eabi-gcc
CFLAGS=-fPIE  -nostdlib -fno-common -fno-stack-protector -fno-exceptions -fno-asynchronous-unwind-tables -fomit-frame-pointer -fno-unwind-tables -Wl,-z,max-page-size=0x04 -Wl,-Ttext-segment=0x00 -nostartfiles -mcpu=cortex-m3 -mthumb -Wl,-eprog_main -L. -lblink -shared -Os

$(progs): %.elf: %.c
	$(CC) $(CFLAGS) $< -o $@

install: $(progs)
	cp -v $(progs) /tmp