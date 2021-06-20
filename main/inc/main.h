#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

#include "sdkconfig.h"

#include <esp_log.h>
#include <esp_event.h>
#include <esp_err.h>
#include <esp_system.h>

#include "ws2812.h"

extern EventGroupHandle_t wifi_event_group, mqtt_event_group;

extern const int CONNECTED_BIT;

extern rgb_led_handle_t led;