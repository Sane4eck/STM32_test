#include "usbd_core.h"
#include "usbd_ctlreq.h"
#include "usbd_conf.h"
#include "stm32g4xx_hal.h"

#define VID 0xCAFE /* Development-only example VID/PID, not registered for distribution */
#define PID 0x4001

static uint8_t device_desc[USB_LEN_DEV_DESC] = {
  18, USB_DESC_TYPE_DEVICE, 0x00, 0x02,
  0x02, 0x02, 0x00, USB_MAX_EP0_SIZE,
  LOBYTE(VID), HIBYTE(VID), LOBYTE(PID), HIBYTE(PID),
  0x00, 0x01, 1, 2, 3, 1
};
static uint8_t langid[] = {4, USB_DESC_TYPE_STRING, 0x09, 0x04};
static uint8_t str[USBD_MAX_STR_DESC_SIZ];
static uint8_t serial_desc[26];

static uint8_t *dev(USBD_SpeedTypeDef s, uint16_t *len) {
  (void)s; *len = sizeof(device_desc); return device_desc;
}
static uint8_t *lang(USBD_SpeedTypeDef s, uint16_t *len) {
  (void)s; *len = sizeof(langid); return langid;
}
static uint8_t *string_desc(USBD_SpeedTypeDef s, uint16_t *len, const char *v) {
  (void)s; USBD_GetString((uint8_t *)v, str, len); return str;
}
static uint8_t *manufacturer(USBD_SpeedTypeDef s, uint16_t *len) {
  return string_desc(s, len, "STM32 Test");
}
static uint8_t *product(USBD_SpeedTypeDef s, uint16_t *len) {
  return string_desc(s, len, "STM32G431 USB CDC");
}
static uint8_t *configuration(USBD_SpeedTypeDef s, uint16_t *len) {
  return string_desc(s, len, "CDC Config");
}
static uint8_t *interface(USBD_SpeedTypeDef s, uint16_t *len) {
  return string_desc(s, len, "CDC Interface");
}
static uint8_t *serial(USBD_SpeedTypeDef s, uint16_t *len) {
  (void)s;
  uint32_t uid[3] = {HAL_GetUIDw0(), HAL_GetUIDw1(), HAL_GetUIDw2()};
  uint32_t value = uid[0] ^ uid[1] ^ uid[2];
  const char hex[]="0123456789ABCDEF";
  serial_desc[0] = 26; serial_desc[1] = USB_DESC_TYPE_STRING;
  for (int i=0; i<12; ++i) {
    uint8_t v=(uint8_t)((value >> ((i%8)*4)) & 0xF);
    serial_desc[2+i*2]=hex[v]; serial_desc[3+i*2]=0;
  }
  *len=sizeof(serial_desc); return serial_desc;
}
USBD_DescriptorsTypeDef USB_Desc = {
  dev, lang, manufacturer, product, serial, configuration, interface
};
