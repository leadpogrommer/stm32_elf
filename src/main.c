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
    size_t symtab_addr = *((size_t *)(0x08000000 + 0x10000 - 8));
    size_t strtab_addr = *((size_t *)(0x08000000 + 0x10000 - 4));

    Elf32_Sym *symtab = (Elf32_Sym *)symtab_addr;
    char *strtab = (char *)strtab_addr;
    uint32_t n_syms = (strtab_addr - symtab_addr) / sizeof(Elf32_Sym);

    printf("symtab addr: %x, strtab addr: %x, n_syms: %lu\n", symtab_addr, strtab_addr, n_syms);

    while (1){
        printf("Enter symbol name: ");
        gets(sym_name);
        Elf32_Sym *sym = 0;
        for(int i = 0; i < n_syms; i++){
            if(strcmp(sym_name, strtab + symtab[i].st_name) == 0){
                sym = symtab + i;
                break;
            }
        }
        if(sym == 0){
            printf("Symbol %s not found\n", sym_name);
            continue;
        }
        printf("Symbol %s addr %08lX\n", sym_name, sym->st_value);
    }

    vTaskDelete(NULL);
}

xTaskHandle blink_task_handle;
xTaskHandle load_elf_task_handle;

int main() {
    rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);

    rcc_periph_clock_enable(RCC_LED_PORT);
    gpio_set_mode(LED_PORT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, LED_PIN);

    dev_decr = usb_setup();
    init_fs();

    while (!is_initialized()) {

    }

    gpio_clear(LED_PORT, LED_PIN);

//    start_fs_test();
    xTaskCreate(blink_task, "Blink task", 256, NULL, 2, &blink_task_handle);
    xTaskCreate(load_elf_task, "ELF", 256, NULL, 2, &load_elf_task_handle);

    // no hostfs IO until scheduler has been started
    vTaskStartScheduler();
}