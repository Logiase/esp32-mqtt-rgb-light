#include "ws2812.h"

#define WS2812_T0H_NS (350)
#define WS2812_T0L_NS (1000)
#define WS2812_T1H_NS (1000)
#define WS2812_T1L_NS (350)
#define WS2812_RESET_US (280)

uint32_t ws2812_t0h_ticks = 0;
uint32_t ws2812_t0l_ticks = 0;
uint32_t ws2812_t1h_ticks = 0;
uint32_t ws2812_t1l_ticks = 0;

static void ws2812_rmt_translator(const void *src, rmt_item32_t *dest, size_t src_size, size_t wanted_num, size_t *translated_size, size_t *item_num)
{
    if (src == NULL || dest == NULL)
    {
        *translated_size = 0;
        *item_num = 0;
        return;
    }

    const rmt_item32_t bit0 = {{{ws2812_t0h_ticks, 1, ws2812_t0l_ticks, 0}}};
    const rmt_item32_t bit1 = {{{ws2812_t1h_ticks, 1, ws2812_t1l_ticks, 0}}};

    uint8_t *psrc = (uint8_t *)src;
    rmt_item32_t *pdest = dest;

    size_t size = 0;
    size_t num = 0;
    while (size < src_size && num < wanted_num)
    {
        for (int i = 0; i < 8; i++)
        {
            if (*psrc & (1 << (7 - i)))
                pdest->val = bit1.val;
            else
                pdest->val = bit0.val;

            num++;
            pdest++;
        }

        size++;
        psrc++;
    }
    *translated_size = size;
    *item_num = num;
}

ws2812_handle_t ws2812_init(int gpio, int channel_id)
{
    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(gpio, channel_id);
    config.clk_div = 2;

    ESP_ERROR_CHECK(rmt_config(&config));
    ESP_ERROR_CHECK(rmt_driver_install((rmt_channel_t)channel_id, 0, 0));

    uint32_t counter_clk_hz = 0;
    ESP_ERROR_CHECK(rmt_get_counter_clock((rmt_channel_t)config.channel, &counter_clk_hz));
    float ratio = (float)counter_clk_hz / 1e9;
    ws2812_t0h_ticks = (uint32_t)(ratio * WS2812_T0H_NS);
    ws2812_t0l_ticks = (uint32_t)(ratio * WS2812_T0L_NS);
    ws2812_t1h_ticks = (uint32_t)(ratio * WS2812_T1H_NS);
    ws2812_t1l_ticks = (uint32_t)(ratio * WS2812_T1L_NS);

    rmt_translator_init((rmt_channel_t)channel_id, ws2812_rmt_translator);

    ws2812_handle_t rgb_led = {
        .channel = (rmt_channel_t)channel_id,
    };

    return rgb_led;
}

void ws2812_set_rgb(ws2812_handle_t *led, rgb_color_t color)
{
    ws2812_set_rgb_raw(led, color.r, color.g, color.b);
}

void ws2812_set_rgb_raw(ws2812_handle_t *led, uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t rgb[] = {g, r, b};
    ESP_ERROR_CHECK(rmt_write_sample(led->channel, rgb, 3 * sizeof(uint8_t), true));
}

void ws2812_set_hsv(ws2812_handle_t *led, uint32_t h, uint32_t s, uint32_t v)
{
    uint8_t r, g, b;

    h %= 360; // h -> [0,360]
    uint32_t rgb_max = v * 2.55f;
    uint32_t rgb_min = rgb_max * (100 - s) / 100.0f;

    uint32_t i = h / 60;
    uint32_t diff = h % 60;

    // RGB adjustment amount by hue
    uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

    switch (i) {
    case 0:
        r = rgb_max;
        g = rgb_min + rgb_adj;
        b = rgb_min;
        break;
    case 1:
        r = rgb_max - rgb_adj;
        g = rgb_max;
        b = rgb_min;
        break;
    case 2:
        r = rgb_min;
        g = rgb_max;
        b = rgb_min + rgb_adj;
        break;
    case 3:
        r = rgb_min;
        g = rgb_max - rgb_adj;
        b = rgb_max;
        break;
    case 4:
        r = rgb_min + rgb_adj;
        g = rgb_min;
        b = rgb_max;
        break;
    default:
        r = rgb_max;
        g = rgb_min;
        b = rgb_max - rgb_adj;
        break;
    }

    ws2812_set_rgb_raw(led, r, g, b);
}

void ws2812_reset(ws2812_handle_t *led)
{
    ws2812_set_rgb_raw(led, 0, 0, 0);
}