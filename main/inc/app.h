#pragma once

#include <mqtt_client.h>

void app_init(void);

void app_mqtt_init(esp_mqtt_client_handle_t client);

void app_send_status(esp_mqtt_client_handle_t client);

void app_receive_cmd(esp_mqtt_event_handle_t event);