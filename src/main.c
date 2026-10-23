#include "stm32g4xx_hal.h"
#include "usb_device.h"
#include "usbd_cdc_if.h"
#include "hall.h"

#include <stdio.h>

#define SAMPLE_INTERVAL_MS 100U
#define QUEUE_CAPACITY     64U  /* 6.4 seconds at 10 Hz */

typedef struct {
    uint32_t seq, t_ms, rpm, period_us, age_ms;
    uint32_t pulses, rejected, overcaptures, dropped;
    uint8_t valid;
} Sample;

static Sample queue[QUEUE_CAPACITY];
static uint8_t rd, wr, size;
static uint32_t next_seq;
static uint32_t dropped;
static char tx_line[128];

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
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) while (1) {}

    clk.PeriphClockSelection = RCC_PERIPHCLK_USB;
    clk.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;
    if (HAL_RCCEx_PeriphCLKConfig(&clk) != HAL_OK) while (1) {}

    __HAL_RCC_CRS_CLK_ENABLE();
    crs.Prescaler = RCC_CRS_SYNC_DIV1;
    crs.Source = RCC_CRS_SYNC_SOURCE_USB;
    crs.Polarity = RCC_CRS_SYNC_POLARITY_RISING;
    crs.ReloadValue = __HAL_RCC_CRS_RELOADVALUE_CALCULATE(48000000, 1000);
    crs.ErrorLimitValue = 34;
    crs.HSI48CalibrationValue = 32;
    HAL_RCCEx_CRSConfig(&crs); /* This HAL function returns void. */
}

static void CollectSample(uint32_t now_ms)
{
    uint32_t seq = next_seq++;
    HallReading hall;
    Hall_GetReading(&hall);

    if (size == QUEUE_CAPACITY) {
        ++dropped;  /* Keep old records; the PC can detect skipped seq IDs. */
        return;
    }

    Sample *s = &queue[wr];
    s->seq = seq;
    s->t_ms = now_ms;
    s->rpm = hall.rpm;
    s->period_us = hall.period_us;
    s->age_ms = hall.age_ms;
    s->pulses = hall.pulses;
    s->rejected = hall.rejected;
    s->overcaptures = hall.overcaptures;
    s->valid = hall.valid;
    s->dropped = dropped;
    wr = (uint8_t)((wr + 1U) % QUEUE_CAPACITY);
    ++size;
}

static void UsbSendPending(void)
{
    if (!size) return;
    const Sample *s = &queue[rd];

    int len = snprintf(tx_line, sizeof(tx_line),
        "%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%u,%lu,NA\r\n",
        (unsigned long)s->seq,
        (unsigned long)s->t_ms,
        (unsigned long)s->rpm,
        (unsigned long)s->period_us,
        (unsigned long)s->pulses,
        (unsigned long)s->rejected,
        (unsigned long)s->overcaptures,
        (unsigned long)s->age_ms,
        (unsigned)s->valid,
        (unsigned long)s->dropped);

    if (len <= 0 || len >= (int)sizeof(tx_line)) return;
    if (CDC_Transmit_FS((uint8_t *)tx_line, (uint16_t)len) == USBD_OK) {
        rd = (uint8_t)((rd + 1U) % QUEUE_CAPACITY);
        --size;
    }
}

int main(void)
{
    HAL_Init();
    USB_Clock_Init();
    if (Hall_Init() != HAL_OK) while (1) {}
    USB_Device_Init();

    uint32_t next_sample_ms = HAL_GetTick() + SAMPLE_INTERVAL_MS;
    while (1) {
        uint32_t now = HAL_GetTick();
        if ((int32_t)(now - next_sample_ms) >= 0) {
            CollectSample(now);
            next_sample_ms += SAMPLE_INTERVAL_MS;
            if ((int32_t)(now - next_sample_ms) >= 0)
                next_sample_ms = now + SAMPLE_INTERVAL_MS;
        }
        UsbSendPending();
    }
}
