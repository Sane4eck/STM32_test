#include "usb_device.h"
#include "usbd_core.h"
#include "usbd_cdc.h"
#include "usbd_cdc_if.h"

USBD_HandleTypeDef hUsbDeviceFS;
extern USBD_DescriptorsTypeDef USB_Desc;

void USB_Device_Init(void)
{
    if (USBD_Init(&hUsbDeviceFS, &USB_Desc, DEVICE_FS) != USBD_OK) while (1) {}
    if (USBD_RegisterClass(&hUsbDeviceFS, &USBD_CDC) != USBD_OK) while (1) {}
    if (USBD_CDC_RegisterInterface(&hUsbDeviceFS, &USBD_Interface_fops_FS) != USBD_OK) while (1) {}
    if (USBD_Start(&hUsbDeviceFS) != USBD_OK) while (1) {}
}
