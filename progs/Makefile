progs = hello.elf calc.elf some_tests.elf

all: $(progs)

CC=arm-none-eabi-gcc
CFLAGS=-fPIE  -nostdlib -fno-common -fno-stack-protector -fno-exceptions -fno-asynchronous-unwind-tables -fomit-frame-pointer -fno-unwind-tables -Wl,-z,max-page-size=0x04 -Wl,-Ttext-segment=0x00 -nostartfiles -mcpu=cortex-m3 -mthumb -Wl,-eprog_main -L. -lblink -shared -Os


libblink.so: ../cmake-build-debug/blink.elf
	cp -v ../cmake-build-debug/blink.elf libblink.so
	elfedit --output-type dyn libblink.so

$(progs): %.elf: %.c libblink.so
	$(CC) $(CFLAGS) $< -o $@

install: $(progs)
	cp -v $(progs) /tmp