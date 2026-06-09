#include "task_app_wifi_manage.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

static const char *WIFI_TAG = "WIFI_MGR";

// 配置您的手机热点SSID和密码
#define WIFI_SSID      "qzm"
#define WIFI_PASS      "130168130168"

// WiFi 状态
static wifi_state_t s_wifi_state = WIFI_STATE_DISCONNECTED;

// FreeRTOS 事件组
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
static EventGroupHandle_t s_wifi_event_group;

// 任务句柄
static TaskHandle_t s_wifi_task_handle = NULL;

// 状态名称
static const char *s_wifi_state_names[] = {
    "DISCONNECTED",
    "CONNECTING",
    "CONNECTED",
    "ERROR"
};

static const char *get_wifi_state_name(wifi_state_t state)
{
    if (state < sizeof(s_wifi_state_names) / sizeof(s_wifi_state_names[0])) {
        return s_wifi_state_names[state];
    }
    return "UNKNOWN";
}

static void set_wifi_state(wifi_state_t new_state)
{
    s_wifi_state = new_state;
}

wifi_state_t app_get_wifi_state(void)
{
    return s_wifi_state;
}

bool app_wifi_is_connected(void)
{
    return s_wifi_state == WIFI_STATE_CONNECTED;
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        set_wifi_state(WIFI_STATE_CONNECTING);
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(WIFI_TAG, "WiFi disconnected, reconnecting...");
        set_wifi_state(WIFI_STATE_DISCONNECTED);
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        esp_wifi_connect();
        set_wifi_state(WIFI_STATE_CONNECTING);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(WIFI_TAG, "WiFi connected, IP: " IPSTR, IP2STR(&event->ip_info.ip));
        set_wifi_state(WIFI_STATE_CONNECTED);
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();
    
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));
    
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    ESP_LOGI(WIFI_TAG, "WiFi station initialized, connecting to %s...", WIFI_SSID);
}

static void task_app_wifi_manage(void *pvParameter)
{
    ESP_LOGI(WIFI_TAG, "WiFi management task started");
    
    // 初始化 NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // 初始化 WiFi
    wifi_init_sta();
    
    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void task_app_wifi_manage_start(void)
{
    BaseType_t ret = xTaskCreate(
        task_app_wifi_manage,
        "task_wifi_manage",
        8192,
        NULL,
        4,
        &s_wifi_task_handle
    );
    
    if (ret == pdPASS) {
        ESP_LOGI(WIFI_TAG, "WiFi management task created");
    } else {
        ESP_LOGE(WIFI_TAG, "Failed to create WiFi management task");
    }
}
