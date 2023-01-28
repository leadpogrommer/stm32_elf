#include <libopencm3/usb/usbd.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <stddef.h>
#include "usb_request.h"
#include <FreeRTOS.h>
#include <task.h>

static const struct usb_device_descriptor dev_decr = {
        .bLength = USB_DT_DEVICE_SIZE,
        .bDescriptorType = USB_DT_DEVICE,
        .bcdUSB = 0x0200,
        .bDeviceClass = 0xff,
        .bDeviceSubClass = 0,
        .bDeviceProtocol = 0,
        .bMaxPacketSize0 = 64,
        .idVendor = 0xCAFE,
        .idProduct = 0xBABE,
        .bcdDevice = 0x1337,
        .iManufacturer = 1,
        .iProduct = 2,
        .iSerialNumber = 3,
        .bNumConfigurations = 1,
};

#define EP_CONTROL USB_ENDPOINT_ADDR_IN(3)
#define EP_DATA_IN USB_ENDPOINT_ADDR_IN(4)
#define EP_DATA_OUT USB_ENDPOINT_ADDR_OUT(4)

static const struct usb_endpoint_descriptor endpoints[] = {
        {
                .bLength = USB_DT_ENDPOINT_SIZE,
                .bDescriptorType = USB_DT_ENDPOINT,
                .bEndpointAddress = EP_CONTROL,
                .bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
                .wMaxPacketSize = 64,
                .bInterval = 1,
        },
        {
                .bLength = USB_DT_ENDPOINT_SIZE,
                .bDescriptorType = USB_DT_ENDPOINT,
                .bEndpointAddress = EP_DATA_OUT,
                .bmAttributes = USB_ENDPOINT_ATTR_BULK,
                .wMaxPacketSize = 64,
                .bInterval = 1,
        },
        {
                .bLength = USB_DT_ENDPOINT_SIZE,
                .bDescriptorType = USB_DT_ENDPOINT,
                .bEndpointAddress = EP_DATA_IN,
                .bmAttributes = USB_ENDPOINT_ATTR_BULK,
                .wMaxPacketSize = 64,
                .bInterval = 1,
        },
};

static const struct usb_interface_descriptor iface = {
        .bLength = USB_DT_INTERFACE_SIZE,
        .bDescriptorType = USB_DT_INTERFACE,
        .bInterfaceNumber = 0,
        .bAlternateSetting = 0,
        .bNumEndpoints = 3,
        // end of descriptor
        .endpoint = endpoints,
};

static const struct usb_interface ifaces[] = {{
                                                      .num_altsetting = 1,
                                                      .altsetting = &iface,
                                              }};

static const struct usb_config_descriptor config = {
        .bLength = USB_DT_CONFIGURATION_SIZE,
        .bDescriptorType = USB_DT_CONFIGURATION,
        .wTotalLength = 0,
        .bNumInterfaces = 1,
        .bConfigurationValue = 1,
        .iConfiguration = 4,
        .bmAttributes = 0x80,
        .bMaxPower = 100, // 200 mA
        // end of descriptor
        .interface = ifaces,
};

static const char *usb_strings[] = {
        "Leadpogrommer",
        "Hostfs client",
        "f-u-c-k",
        "Default (and only) configuration",
};

static uint8_t control_buffer[128];


static int inited = 0;

static enum usbd_request_return_codes
dummy_control_callback(usbd_device *usbd_dev, struct usb_setup_data *req, uint8_t **buf,
                       uint16_t *len, void (**complete)(usbd_device *usbd_dev, struct usb_setup_data *req)) {
    if (req->bmRequestType != 0x40)
        return USBD_REQ_NOTSUPP; /* Only accept vendor request. */

    inited = 1;

    return USBD_REQ_HANDLED;
}


static void data_write_cb(usbd_device *dev, uint8_t ep);
static void data_read_cb(usbd_device *dev, uint8_t ep);
static void send_interrupt_callback(usbd_device *usbd_dev, uint8_t ep);
static void usb_set_config_cb(usbd_device *usbd_dev, uint16_t value) {
    usbd_register_control_callback(usbd_dev, USB_REQ_TYPE_VENDOR, USB_REQ_TYPE_TYPE, dummy_control_callback);
    usbd_ep_setup(usbd_dev, USB_ENDPOINT_ADDR_IN(3), USB_ENDPOINT_ATTR_INTERRUPT, 64, send_interrupt_callback);
    usbd_ep_setup(usbd_dev, EP_DATA_OUT, USB_ENDPOINT_ATTR_BULK, 64, data_read_cb);
    usbd_ep_setup(usbd_dev, EP_DATA_IN, USB_ENDPOINT_ATTR_BULK, 64, data_write_cb);
//    inited = 1;
}

int is_initialized() {
    return inited;
}

static usbd_device *usbd_dev;

void usb_wakeup_isr(void) {
    usbd_poll(usbd_dev);
}

void usb_lp_can_rx0_isr(void) {
    usbd_poll(usbd_dev);
}

void usb_hp_can_tx_isr(void){
    usbd_poll(usbd_dev);
}

struct {
    request_buffer_t *send_buffer;
    request_buffer_t *receive_buffers;
    int request_buffers_count;
    int current_buffer;
    char done_reading;
    char  done_writing;
    TaskHandle_t task_to_notify;
}request_state;

//ISR
static void check_and_notify(){
    if(request_state.done_reading && request_state.done_writing){
        vTaskNotifyGiveFromISR(request_state.task_to_notify, NULL);
    }
}

// ISR
static void data_read_cb(usbd_device *dev, uint8_t ep){
    if(request_state.done_reading)return;
    if(request_state.request_buffers_count == 0){
        request_state.done_reading = 1;
        check_and_notify();
        return;
    }
    request_buffer_t *buff = &request_state.receive_buffers[request_state.current_buffer];
    size_t to_read = buff->len - buff->pos;
    if(to_read > 64){
        to_read = 64;
    }
    uint16_t  was_read = usbd_ep_read_packet(usbd_dev, EP_DATA_OUT, buff->buff+buff->pos, to_read);
    buff->pos += was_read;
    if(was_read == 0){
        request_state.current_buffer++;
        if(request_state.current_buffer == request_state.request_buffers_count){
            request_state.done_reading = 1;
            check_and_notify();
        }
    }
}

// ISR
static void send_interrupt_callback(usbd_device *dev, uint8_t ep) {
    // we just have sent our request, now we start writing data
    data_write_cb(usbd_dev, EP_DATA_IN);
}

// ISR
static void data_write_cb(usbd_device *dev, uint8_t ep){
    if(request_state.done_writing)return;
    if(request_state.send_buffer == 0){
        usbd_ep_write_packet(usbd_dev, EP_DATA_IN, 0, 0);
        request_state.done_writing = 1;
        check_and_notify();
        return;
    }
    uint16_t to_write = request_state.send_buffer->len - request_state.send_buffer->pos;
    if(to_write > 64){
        to_write = 64;
    }

    usbd_ep_write_packet(usbd_dev, EP_DATA_IN, request_state.send_buffer->buff + request_state.send_buffer->pos, to_write);
    request_state.send_buffer->pos += to_write;
    if(to_write == 0){
        request_state.done_writing = 1;
        check_and_notify();
    }
}


void usb_make_request(void *command, size_t command_size, request_buffer_t* send_buffer, request_buffer_t* receive_buffers, int request_buffer_count){
    // setup request data
    request_state.done_reading = 0;
    request_state.done_writing = 0;
    request_state.send_buffer = send_buffer;
    request_state.receive_buffers = receive_buffers;
    request_state.request_buffers_count = request_buffer_count;
    request_state.current_buffer = 0;
    request_state.task_to_notify = xTaskGetCurrentTaskHandle();
    for(int i = 0; i < request_buffer_count; i++){
        receive_buffers[i].pos = 0;
    }
    if(send_buffer)send_buffer->pos = 0;


    // initiate request
    usbd_ep_write_packet(usbd_dev, EP_CONTROL, command, command_size);

    // and now we wait
    xTaskNotifyWait(~0, 0, NULL, 9999999);
}


usbd_device *usb_setup(void) {
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_USB);

    usbd_dev = usbd_init(&st_usbfs_v1_usb_driver, &dev_decr, &config, usb_strings, 4, control_buffer,
                         sizeof(control_buffer));

    nvic_enable_irq(NVIC_USB_LP_CAN_RX0_IRQ);
    nvic_enable_irq(NVIC_USB_HP_CAN_TX_IRQ);
    nvic_enable_irq(NVIC_USB_WAKEUP_IRQ);

    usbd_register_set_config_callback(usbd_dev, usb_set_config_cb);

    return usbd_dev;
}