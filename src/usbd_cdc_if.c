#include "usbd_cdc_if.h"
#include "usb_device.h"
#include <string.h>

static uint8_t rxbuf[64];
static uint8_t txbuf[128];
static volatile uint8_t ready = 0;
static USBD_CDC_LineCodingTypeDef coding = {115200, 0, 0, 8};

static int8_t cdc_init(void)
{
    USBD_CDC_SetTxBuffer(&hUsbDeviceFS, txbuf, 0);
    USBD_CDC_SetRxBuffer(&hUsbDeviceFS, rxbuf);
    ready = 1;
    return USBD_OK;
}
static int8_t cdc_deinit(void) { ready = 0; return USBD_OK; }
static int8_t cdc_control(uint8_t cmd, uint8_t *buf, uint16_t len)
{
    (void)len;
    if (cmd == CDC_SET_LINE_CODING) {
        coding.bitrate = (uint32_t)buf[0] | ((uint32_t)buf[1]<<8) |
                         ((uint32_t)buf[2]<<16) | ((uint32_t)buf[3]<<24);
        coding.format = buf[4]; coding.paritytype = buf[5]; coding.datatype = buf[6];
    } else if (cmd == CDC_GET_LINE_CODING) {
        buf[0] = coding.bitrate; buf[1] = coding.bitrate>>8;
        buf[2] = coding.bitrate>>16; buf[3] = coding.bitrate>>24;
        buf[4] = coding.format; buf[5] = coding.paritytype; buf[6] = coding.datatype;
    }
    return USBD_OK;
}
static int8_t cdc_receive(uint8_t *buf, uint32_t *len)
{
    (void)buf; (void)len;
    USBD_CDC_SetRxBuffer(&hUsbDeviceFS, rxbuf);
    USBD_CDC_ReceivePacket(&hUsbDeviceFS);
    return USBD_OK;
}
USBD_CDC_ItfTypeDef USBD_Interface_fops_FS = {
    cdc_init, cdc_deinit, cdc_control, cdc_receive
};

uint8_t CDC_Transmit_FS(uint8_t *data, uint16_t len)
{
    if (!ready || hUsbDeviceFS.dev_state != USBD_STATE_CONFIGURED || len > sizeof(txbuf)) return USBD_BUSY;
    USBD_CDC_HandleTypeDef *cdc = (USBD_CDC_HandleTypeDef*)hUsbDeviceFS.pClassData;
    if (!cdc || cdc->TxState) return USBD_BUSY;
    memcpy(txbuf, data, len);
    USBD_CDC_SetTxBuffer(&hUsbDeviceFS, txbuf, len);
    return USBD_CDC_TransmitPacket(&hUsbDeviceFS);
}
