

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <stdio.h>
#include <libopencm3/usb/usbd.h>
#include <string.h>
#include "usb_cdc.h"

#define LED_PORT GPIOC
#define RCC_LED_PORT RCC_GPIOC
#define LED_PIN GPIO13

usbd_device* dev;

char buff[1024];

int main() {
    rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);

  rcc_periph_clock_enable(RCC_LED_PORT);
  gpio_set_mode(LED_PORT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, LED_PIN);


//  printf("Hell\n");

 dev = usb_setup();

    while (!is_initialized()){

    }

    gpio_clear(LED_PORT, LED_PIN);

  int i = 0;
  while(1) {
////    usbd_poll(dev);c
//      printf("Hello %d\r\n", i++);
//      usbd_poll(dev);
      sprintf(buff, "%d ticks", i++);
      while(!usbd_ep_write_packet(dev, USB_ENDPOINT_ADDR_IN(3), buff, strlen(buff)));
  }
}

//int _write (int    file,
//        char * ptr,
//        int    len){
//    usbd_ep_write_packet(dev, 0x82, ptr, len);
//    return len;
//}