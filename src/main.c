

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <stdio.h>
#include <libopencm3/usb/usbd.h>
#include "usb_cdc.h"

#define LED_PORT GPIOC
#define RCC_LED_PORT RCC_GPIOC
#define LED_PIN GPIO13

usbd_device* dev;

int main() {
    rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);

  rcc_periph_clock_enable(RCC_LED_PORT);
  gpio_set_mode(LED_PORT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, LED_PIN);

  gpio_set(LED_PORT, LED_PIN);
//  printf("Hell\n");

  dev = usb_setup();

  int i = 0;
  while(1) {
//    usbd_poll(dev);
      printf("Hello %d\r\n", i++);
      usbd_poll(dev);
  }
}

int _write (int    file,
        char * ptr,
        int    len){
    usbd_ep_write_packet(dev, 0x82, ptr, len);
    return len;
}