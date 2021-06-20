#include "app.h"
#include "ws2812.h"
#include "sdkconfig.h"

#include <esp_log.h>
#include <cJSON.h>

static const char *TAG = "APP";

ws2812_handle_t ws2812;

uint32_t h = 0;
uint32_t s = 0;
uint32_t brightness = 0;
bool state = false;

void app_init()
{
    ws2812 = ws2812_init(45, 1);
    ws2812_reset(&ws2812);
}

void app_mqtt_init(esp_mqtt_client_handle_t client)
{
    esp_mqtt_client_subscribe(client, CONFIG_MQTT_APP_SET_TOPIC, 0);
}

void app_send_status(esp_mqtt_client_handle_t client)
{
    cJSON *pRoot = cJSON_CreateObject();

    if (state == false)
    {
        cJSON_AddStringToObject(pRoot, "state", "OFF");
        esp_mqtt_client_publish(client, CONFIG_MQTT_APP_STATUS_TOPIC, cJSON_Print(pRoot), 0, 0, 0);
        return;
    }

    cJSON_AddStringToObject(pRoot, "state", "ON");
    cJSON *pColor = cJSON_CreateObject();
    cJSON_AddStringToObject(pRoot, "color_mode", "hs");
    cJSON_AddNumberToObject(pColor, "h", h);
    cJSON_AddNumberToObject(pColor, "s", s);
    cJSON_AddItemToObject(pRoot, "color", pColor);
    cJSON_AddNumberToObject(pRoot, "brightness", brightness);

    esp_mqtt_client_publish(client, CONFIG_MQTT_APP_STATUS_TOPIC, cJSON_Print(pRoot), 0, 0, 0);
    return;
}

void app_receive_cmd(esp_mqtt_event_handle_t event)
{
    char data[CONFIG_MQTT_APP_BUFFER_DEFAULT_SIZE];

    memset(data, 0, CONFIG_MQTT_APP_BUFFER_DEFAULT_SIZE);
    strncpy(data, event->data, event->data_len);

    ESP_LOGI(TAG, "Received Data %s", data);

    cJSON *pRoot = cJSON_Parse(data);
    if (!pRoot)
        goto wrong;

    cJSON *pState = cJSON_GetObjectItem(pRoot, "state");
    if (!pState || !cJSON_IsString(pState))
        goto wrong;

    if (strcmp(pState->valuestring, "OFF") == 0)
    {
        state = false;
        ws2812_set_rgb_raw(&ws2812, 0, 0, 0);
        app_send_status(event->client);
        return;
    }

    if (strcmp(pState->valuestring, "ON") != 0)
        goto wrong;
    
    state = true;
    cJSON *pColor = cJSON_GetObjectItem(pRoot, "color");
    if (pColor)
    {
        cJSON *pH = cJSON_GetObjectItem(pColor, "h");
        cJSON *pS = cJSON_GetObjectItem(pColor, "s");

        if (!pH || !pS || !cJSON_IsNumber(pH) || !cJSON_IsNumber(pS))
            goto wrong;

        h = (uint32_t)pH->valuedouble;
        s = (uint32_t)pS->valuedouble;
    }

    cJSON *pBrightness = cJSON_GetObjectItem(pRoot, "brightness");
    if (pBrightness)
    {
        if (!cJSON_IsNumber(pBrightness))
            goto wrong;
        brightness = (uint32_t)pBrightness->valueint;
    }

    ws2812_set_hsv(&ws2812, h, s, brightness);
    app_send_status(event->client);
    return;

wrong:
    ESP_LOGE(TAG, "payload format wrong!");
    return;
}