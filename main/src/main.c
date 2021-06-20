#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

#include "sdkconfig.h"

#include <esp_log.h>
#include <esp_event.h>
#include <esp_err.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <mqtt_client.h>
#include <nvs_flash.h>

#include <ws2812.h>

#include "app.h"

// event groups
EventGroupHandle_t wifi_event_group, mqtt_event_group;

const int CONNECTED_BIT = BIT0;

static const char *TAG = "MAIN";

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event);
static void mqtt_start(void);

static void wifi_event_handler(void *handler_arg, esp_event_base_t base, int32_t id, void *event_data);
static void ip_event_handler(void *arg, esp_event_base_t base, int32_t id, void *event_data);
static void wifi_init_sta(void);

void app_main()
{
    ESP_LOGI(TAG, "start ...");
    ESP_LOGI(TAG, "Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "IDF Version: %s", esp_get_idf_version());

    app_init();

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    wifi_init_sta();
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
    mqtt_start();
}

static void wifi_init_sta(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, ip_event_handler, NULL));

    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSWORD,
        }
    };

    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

static void wifi_event_handler(void *handler_arg, esp_event_base_t base, int32_t id, void *event_data) {
    switch (id) {
        case WIFI_EVENT_STA_START:
            ESP_LOGI(TAG, "[WIFI] Connecting to %s", CONFIG_WIFI_SSID);
            ESP_ERROR_CHECK(esp_wifi_connect());
            // wifi_err_handler(err);
            break;

        case WIFI_EVENT_STA_CONNECTED:
            ESP_LOGI(TAG, "[WIFI] Connected.");
            break;

        case WIFI_EVENT_STA_DISCONNECTED:
            ESP_LOGW(TAG, "[WIFI] Disconnected.");
            ESP_LOGI(TAG, "[WIFI] Reconnecting to %s ...", CONFIG_WIFI_SSID);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            break;

        default:
            ESP_LOGI(TAG, "[WIFI] Event base %s with ID %d", base, id);
            break;
    }
}

static void ip_event_handler(void *arg, esp_event_base_t base, int32_t id, void *event_data) {
    if (id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG, "[IP] Got IP: "
        IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
    } else {
        ESP_LOGI(TAG, "[IP] Event base %s with ID %d", base, id);
    }
}

static void mqtt_start(void)
{
    mqtt_event_group = xEventGroupCreate();

    const esp_mqtt_client_config_t mqtt_config = {
        .client_id = CONFIG_MQTT_CLIENT_ID,
        .uri = CONFIG_MQTT_URI,
        .username = CONFIG_MQTT_USERNAME,
        .password = CONFIG_MQTT_PASSWORD,
        .event_handle = mqtt_event_handler,
        .keepalive = 10,
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_config);
    ESP_ERROR_CHECK(esp_mqtt_client_start(client));
    ESP_LOGI(TAG, "[MQTT] Connecting to %s ...", CONFIG_MQTT_URI);
}

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    char topic[CONFIG_MQTT_APP_BUFFER_DEFAULT_SIZE] = {0};
    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "[MQTT] Connected.");
        app_mqtt_init(event->client);
        app_send_status(event->client);
        xEventGroupSetBits(mqtt_event_group, CONNECTED_BIT);
        break;

    case MQTT_EVENT_DISCONNECTED:
        xEventGroupClearBits(mqtt_event_group, CONNECTED_BIT);
        ESP_LOGI(TAG, "[MQTT] Disconnected.");
        break;
    
    case MQTT_EVENT_DATA:
        strncpy(topic, event->topic, event->topic_len);

        ESP_LOGI(TAG, "[MQTT] receive from topic %s", topic);

        if (strcmp(topic, CONFIG_MQTT_APP_SET_TOPIC) == 0)
            app_receive_cmd(event);
        
        break;

    default:
        ESP_LOGI(TAG, "[MQTT] event id %d", event->event_id);
        break;
    }
    return ESP_OK;
}