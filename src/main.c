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
//#include <newlib.h>
#define LED_PORT GPIOC
#define RCC_LED_PORT RCC_GPIOC
#define LED_PIN GPIO13

usbd_device *dev_decr;

//char recv_buff1[1024];
//char recv_buff0[4];
//char *to_send = "Hello world. lorem ipsum dolor sit amet ... http://radiodetali-nsk.ru/wp-content/uploads/2023/01/price-1.xlsx";
//char *cmd = "GIVE_ME_DATA";

char buff[200];

#define BL_1 10
#define BL_2 150

void printing_task(){
//    request_buffer_t send_buffer_sec={to_send, strlen(to_send)};
//    request_buffer_t recive_buffers_desc[2]={{recv_buff0, 4}, {recv_buff1, 1024}};
//
//    usb_make_request(cmd, strlen(cmd), &send_buffer_sec, recive_buffers_desc, 2);

//    printf("Hello, world!\n");
//    vTaskDelay(300000);

    printf("Ppress enter:");
    getchar();
    printf("Starting test...\n");
    FILE *data_file = fopen("/h/data.txt", "r");
    uint32_t read_len = 0;
    FILE *res_file = fopen("/h/r1.txt", "w");
    while ((read_len = fread(buff, 1, 10, data_file))){
        for(uint32_t i = 0; i < read_len; i++){
            buff[i] = toupper(buff[i]);
        }
        fwrite(buff, 1, read_len, res_file);
    }
    fclose(res_file);
    res_file = fopen("/h/r2.txt", "w");
    fseek(data_file, 0, SEEK_SET);
    while ((read_len = fread(buff, 1, 150, data_file))){
        for(uint32_t i = 0; i < read_len; i++){
            buff[i] = tolower(buff[i]);
        }
        fwrite(buff, 1, read_len, res_file);
    }

    fclose(res_file);
    printf("Done\n");
    printf("Free heap: %d\n", xPortGetFreeHeapSize());

    vTaskDelete(NULL);
}

void blink_task(){
    for(;;){
        gpio_toggle(LED_PORT, LED_PIN);
//        printf("LED is %s\n", gpio_get(LED_PORT, LED_PIN) ? "ON" : "OFF");
        vTaskDelay(pdMS_TO_TICKS(333));
    }
}

xTaskHandle test_task_handle;
xTaskHandle blink_task_handle;

int main() {
    rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);

    rcc_periph_clock_enable(RCC_LED_PORT);
    gpio_set_mode(LED_PORT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, LED_PIN);

    dev_decr = usb_setup();
    init_fs();

    while (!is_initialized()) {

    }




    gpio_clear(LED_PORT, LED_PIN);

    xTaskCreate(printing_task, "Test task", 2048, NULL, 2, &test_task_handle);
    xTaskCreate(blink_task, "Blink task", 256, NULL, 2, &blink_task_handle);

    // no hostfs IO until scheduler has been started
    vTaskStartScheduler();
}