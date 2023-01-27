#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <stdio.h>
#include <libopencm3/usb/usbd.h>
#include <string.h>
#include "usb_cdc.h"
#include <FreeRTOS.h>
#include "task.h"
//#include <newlib.h>
#define LED_PORT GPIOC
#define RCC_LED_PORT RCC_GPIOC
#define LED_PIN GPIO13

usbd_device *dev_decr;

char recv_buff1[1024];
char recv_buff0[4];
char *to_send = "Hello world. lorem ipsum dolor sit amet ... http://radiodetali-nsk.ru/wp-content/uploads/2023/01/price-1.xlsx";
char *cmd = "GIVE_ME_DATA";


void printing_task(){
//    int i = 0;
//    while (1) {
//        sprintf(buff, "%d ticks", i++);
//        while (!usbd_ep_write_packet(dev_decr, USB_ENDPOINT_ADDR_IN(3), buff, strlen(buff)));
//        vTaskDelay(pdMS_TO_TICKS(1000));
//    }
//    vTaskDelay(pdMS_TO_TICKS(3000));


    request_buffer_t send_buffer_sec={to_send, strlen(to_send)};
    request_buffer_t recive_buffers_desc[2]={{recv_buff0, 4}, {recv_buff1, 1024}};

    usb_make_request(cmd, strlen(cmd), &send_buffer_sec, recive_buffers_desc, 2);


    vTaskDelay(300000);
}

void blink_task(){
    for(;;){
        gpio_toggle(LED_PORT, LED_PIN);
        vTaskDelay(pdMS_TO_TICKS(333));
//        usbd_poll(dev_decr);
    }
}

xTaskHandle test_task_handle;
xTaskHandle blink_task_handle;

int main() {
    rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);

    rcc_periph_clock_enable(RCC_LED_PORT);
    gpio_set_mode(LED_PORT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, LED_PIN);

    dev_decr = usb_setup();

    while (!is_initialized()) {

    }


    gpio_clear(LED_PORT, LED_PIN);

    xTaskCreate(printing_task, "Test task", 2048, NULL, 2, &test_task_handle);
    xTaskCreate(blink_task, "Blink task", 256, NULL, 2, &blink_task_handle);


    vTaskStartScheduler();



}


// fs api
// -----------------
// close
// fstat
// isatty (stub is enough)
// lseek
// open
// read
// stat
// unlink
// write