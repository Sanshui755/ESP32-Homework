#include "task_app_mqtt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_event.h"
#include "mqtt_client.h"
#include "task_app_wifi_manage.h"
#include "task_app_mainbusiness.h"
#include "sensor_data_mgr.h"
#include <string.h>

static const char *MQTT_TAG = "MQTT_MGR";

// MQTT 配置
#define MQTT_BROKER_URI "mqtt://broker.emqx.io:1883"
#define MQTT_TOPIC "2023108380403_dhb_evt"  // 发布传感器数据的主题
#define MQTT_TOPIC_LAMP "2023108380403_dhb_cmd"  // 发布灯状态的命令主题

static esp_mqtt_client_handle_t s_mqtt_client = NULL;
static TaskHandle_t s_mqtt_task_handle = NULL;
static bool s_mqtt_connected = false;

bool app_mqtt_is_connected(void)
{
    return s_mqtt_connected;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(MQTT_TAG, "Event dispatched from event loop base=%s, event_id=%ld", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(MQTT_TAG, "MQTT connected");
            s_mqtt_connected = true;
            esp_mqtt_client_subscribe(client, MQTT_TOPIC_LAMP, 1);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(MQTT_TAG, "MQTT disconnected");
            s_mqtt_connected = false;
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGD(MQTT_TAG, "MQTT subscribed, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGD(MQTT_TAG, "MQTT unsubscribed, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGD(MQTT_TAG, "MQTT published, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGD(MQTT_TAG, "MQTT data received");
            if (event->topic_len == strlen(MQTT_TOPIC_LAMP) &&
                strncmp(event->topic, MQTT_TOPIC_LAMP, event->topic_len) == 0) {
                if (event->data_len >= 1) {
                    if (((char*)event->data)[0] == '1') {
                        app_send_command("LED_ON", NULL);
                    } else if (((char*)event->data)[0] == '0') {
                        app_send_command("LED_OFF", NULL);
                    }
                }
            }
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(MQTT_TAG, "MQTT error");
            if (event->error_handle) {
                ESP_LOGE(MQTT_TAG, "Error type: %d", event->error_handle->error_type);
            }
            break;
        case MQTT_EVENT_BEFORE_CONNECT:
            break;
        default:
            break;
    }
}

static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
        // 先用MQTT3.1.1测试，后续再切换回5.0
        .session.protocol_ver = MQTT_PROTOCOL_V_3_1_1,
    };
    ESP_LOGI(MQTT_TAG, "Starting MQTT client with broker: %s", MQTT_BROKER_URI);
    s_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(s_mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(s_mqtt_client);
}

static void task_app_mqtt(void *pvParameter)
{
    ESP_LOGI(MQTT_TAG, "MQTT task started");
    
    // 等待WiFi连接
    ESP_LOGI(MQTT_TAG, "Waiting for WiFi connection...");
    while (!app_wifi_is_connected()) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    ESP_LOGI(MQTT_TAG, "WiFi connected, starting MQTT client...");
    
    // 启动 MQTT 客户端
    mqtt_app_start();
    
    // JSON buffer (256 bytes should be enough)
    char json_buf[256];
    
    while (1) {
        // 如果已连接，每 3 秒发送一次数据
        if (s_mqtt_connected) {
            // 获取传感器数据JSON
            int json_len = sensor_data_mgr_get_json(json_buf, sizeof(json_buf));
            if (json_len > 0) {
                // 发布消息（不打印详细日志）
                int msg_id = esp_mqtt_client_publish(s_mqtt_client, MQTT_TOPIC, json_buf, 0, 1, 0);
                if (msg_id == -1) {
                    ESP_LOGE(MQTT_TAG, "Failed to publish");
                }
            }
        }
        
        // 等待 3 秒
        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
}

void task_app_mqtt_start(void)
{
    BaseType_t ret = xTaskCreate(
        task_app_mqtt,
        "task_mqtt",
        8192,
        NULL,
        3,
        &s_mqtt_task_handle
    );
    
    if (ret == pdPASS) {
        ESP_LOGI(MQTT_TAG, "MQTT task created");
    } else {
        ESP_LOGE(MQTT_TAG, "Failed to create MQTT task");
    }
}
