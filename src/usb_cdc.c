#include <libopencm3/usb/usbd.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>

static const struct usb_device_descriptor dev = {
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

static const struct usb_endpoint_descriptor ep3 = {
        .bLength = USB_DT_ENDPOINT_SIZE,
        .bDescriptorType = USB_DT_ENDPOINT,
        .bEndpointAddress = USB_ENDPOINT_ADDR_IN(3),
        .bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
        .wMaxPacketSize = 64,
        .bInterval = 1,
};

static const struct usb_interface_descriptor iface = {
        .bLength = USB_DT_INTERFACE_SIZE,
        .bDescriptorType = USB_DT_INTERFACE,
        .bInterfaceNumber = 0,
        .bAlternateSetting = 0,
        .bNumEndpoints = 1,
        // end of descriptor
        .endpoint = &ep3,
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


static enum usbd_request_return_codes dummy_control_callback(usbd_device *usbd_dev, struct usb_setup_data *req, uint8_t **buf,
                                                             uint16_t *len, void (**complete)(usbd_device *usbd_dev, struct usb_setup_data *req)){
    return USBD_REQ_NOTSUPP;
}

static void wtf_should_never_be_called(usbd_device *usbd_dev, uint8_t ep){}

static int inited = 0;

static void usb_set_config_cb(usbd_device *usbd_dev, uint16_t value){
    usbd_register_control_callback(usbd_dev, USB_REQ_TYPE_VENDOR, USB_REQ_TYPE_TYPE, dummy_control_callback);
    usbd_ep_setup(usbd_dev, USB_ENDPOINT_ADDR_IN(3), USB_ENDPOINT_ATTR_INTERRUPT, 64, wtf_should_never_be_called);
    inited = 1;
}

int is_initialized(){
    return inited;
}

static usbd_device *usbd_dev;

void usb_wakeup_isr(void) {
    usbd_poll(usbd_dev);
}

void usb_lp_can_rx0_isr(void) {
    usbd_poll(usbd_dev);
}

usbd_device* usb_setup(void){
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_USB);

    nvic_enable_irq(NVIC_USB_LP_CAN_RX0_IRQ);
    nvic_enable_irq(NVIC_USB_WAKEUP_IRQ);

    usbd_dev = usbd_init(&st_usbfs_v1_usb_driver, &dev, &config, usb_strings, 4, control_buffer, sizeof(control_buffer));
    usbd_register_set_config_callback(usbd_dev, usb_set_config_cb);



    return usbd_dev;
}