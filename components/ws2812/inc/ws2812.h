#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <freertos/FreeRTOS.h>
#include <driver/rmt.h>

typedef struct {
    rmt_channel_t channel;
} ws2812_handle_t;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgb_color_t;

ws2812_handle_t ws2812_init(int gpio, int channel_id);

void ws2812_set_hsv(ws2812_handle_t *led, uint32_t h, uint32_t s, uint32_t v);

void ws2812_set_rgb(ws2812_handle_t *led, rgb_color_t color);

void ws2812_set_rgb_raw(ws2812_handle_t *led, uint8_t r, uint8_t g, uint8_t b);

void ws2812_reset(ws2812_handle_t *led);

#ifdef __cplusplus
}
#endif