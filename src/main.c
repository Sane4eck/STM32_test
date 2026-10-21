#include "stm32g4xx_hal.h"
#include "usb_device.h"
#include "usbd_cdc_if.h"

#include <stdio.h>
#include <string.h>

void SysTick_Handler(void)
{
    HAL_IncTick();
}

static void USB_Clock_Init(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_PeriphCLKInitTypeDef clk = {0};
    RCC_CRSInitTypeDef crs = {0};

    osc.OscillatorType = RCC_OSCILLATORTYPE_HSI48;
    osc.HSI48State = RCC_HSI48_ON;

    if (HAL_RCC_OscConfig(&osc) != HAL_OK)
        while (1) {}

    clk.PeriphClockSelection = RCC_PERIPHCLK_USB;
    clk.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;

    if (HAL_RCCEx_PeriphCLKConfig(&clk) != HAL_OK)
        while (1) {}

    __HAL_RCC_CRS_CLK_ENABLE();

    crs.Prescaler = RCC_CRS_SYNC_DIV1;
    crs.Source = RCC_CRS_SYNC_SOURCE_USB;
    crs.Polarity = RCC_CRS_SYNC_POLARITY_RISING;
    crs.ReloadValue =
        __HAL_RCC_CRS_RELOADVALUE_CALCULATE(48000000, 1000);
    crs.ErrorLimitValue = 34;
    crs.HSI48CalibrationValue = 32;

    HAL_RCCEx_CRSConfig(&crs);
}

int main(void)
{
    HAL_Init();

    USB_Clock_Init();
    MX_USB_DEVICE_Init();

    HAL_Delay(1500);

    uint32_t counter = 0;
    static char message[80];

    while (1)
    {
        int len = snprintf(
            message,
            sizeof(message),
            "Counter:%lu,Time:%lu\r\n",
            (unsigned long)counter++,
            (unsigned long)HAL_GetTick()
        );

        if (len > 0 && len < sizeof(message))
        {
            CDC_Transmit_FS((uint8_t *)message, (uint16_t)len);
        }

        HAL_Delay(1000);
    }
}
