#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <stdio.h>
#include <libopencm3/usb/usbd.h>
#include <string.h>
#include "usb_request.h"
#include <FreeRTOS.h>
#include <ctype.h>
#include "task.h"
#include "fs.h"
#include "fs_test.h"
#include "exec.h"
#include <elf.h>
//#include <newlib.h>
#define LED_PORT GPIOC
#define RCC_LED_PORT RCC_GPIOC
#define LED_PIN GPIO13

usbd_device *dev_decr;



#define BL_1 10
#define BL_2 150


void blink_task(){
    for(;;){
        gpio_toggle(LED_PORT, LED_PIN);
        vTaskDelay(pdMS_TO_TICKS(333));
    }
}



char sym_name[100];
void load_elf_task(){
    size_t symtab_addr = *((size_t *)(0x08000000 + 0x80000 - 8));
    size_t strtab_addr = *((size_t *)(0x08000000 + 0x80000 - 4));

    Elf32_Sym *symtab = (Elf32_Sym *)symtab_addr;
    char *strtab = (char *)strtab_addr;
    uint32_t n_syms = (strtab_addr - symtab_addr) / sizeof(Elf32_Sym);

    printf("symtab addr: %x, strtab addr: %x, n_syms: %lu\n", symtab_addr, strtab_addr, n_syms);

    while (1){
        printf("(s - find symbol, l - load elf): ");
        char cmd;
        scanf("%c", &cmd);
        while (getchar() != '\n');
        if(cmd == 'l'){
            gets(sym_name);
            exec_file(sym_name);
            continue;
        } else if(cmd != 's'){
            continue;
        }

        printf("Enter symbol name: ");
        gets(sym_name);
        size_t sym = find_symbol(sym_name);
        if(sym == 0){
            printf("Unable to find symbol %s\n", sym_name);
            continue;
        }
        printf("Symbol %s addr %08lX\n", sym_name, sym);
    }

    vTaskDelete(NULL);
}

xTaskHandle blink_task_handle;
xTaskHandle load_elf_task_handle;

int main() {
    rcc_clock_setup_pll(&rcc_hse_25mhz_3v3[RCC_CLOCK_3V3_96MHZ]);

    rcc_periph_clock_enable(RCC_LED_PORT);
    gpio_mode_setup(LED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_PIN);



    dev_decr = usb_setup();
    init_fs();

    while (!is_initialized()) {

    }

    gpio_clear(LED_PORT, LED_PIN);

//    start_fs_test();
    xTaskCreate(blink_task, "Blink task", 256, NULL, 2, &blink_task_handle);
    xTaskCreate(load_elf_task, "ELF", 512, NULL, 2, &load_elf_task_handle);

    // no hostfs IO until scheduler has been started
    vTaskStartScheduler();
}