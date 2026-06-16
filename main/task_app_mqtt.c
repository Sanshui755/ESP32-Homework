#include "task_app_mqtt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_task_wdt.h"
#include "esp_mac.h"
#include "mqtt_client.h"
#include "task_app_wifi_manage.h"
#include "task_app_cmd.h"
#include "task_monitor.h"
#include "sensor_data_mgr.h"
#include <string.h>

static const char *MQTT_TAG = "MQTT_MGR";

#define QZM_IOTGW_BROKER        "mqtt://broker.emqx.io:1883"
#define QZM_IOTGW_SENSOR_DATA   "qzm999/iotgw/sensor/data"
#define QZM_IOTGW_LED_CONTROL   "qzm999/iotgw/led/control"
#define QZM_IOTGW_STATUS        "qzm999/iotgw/status"

#define JSON_BUF_SIZE 512

static esp_mqtt_client_handle_t s_mqtt_client = NULL;
static TaskHandle_t s_mqtt_task_handle = NULL;
static volatile bool s_mqtt_connected = false;

// Cached JSON for re-publish after reconnect
static char s_cached_json[JSON_BUF_SIZE];
static int s_cached_json_len = 0;
static char s_client_id[32];

bool app_mqtt_is_connected(void)
{
    return s_mqtt_connected;
}

void app_mqtt_notify_new_data(void)
{
    if (s_mqtt_task_handle == NULL) return;
    xTaskNotify(s_mqtt_task_handle, 0, eNoAction);
}

bool app_mqtt_publish_data(const char *json, int len)
{
    if (!s_mqtt_client || !s_mqtt_connected) return false;
    if (!json || len <= 0) return false;

    int msg_id = esp_mqtt_client_publish(s_mqtt_client, QZM_IOTGW_SENSOR_DATA, json, len, 1, 0);
    if (msg_id != -1) {
        memcpy(s_cached_json, json, len);
        s_cached_json_len = len;
        ESP_LOGI(MQTT_TAG, "→ PUB [%s] qos=1 len=%d msg_id=%d", QZM_IOTGW_SENSOR_DATA, len, msg_id);
        return true;
    }
    ESP_LOGW(MQTT_TAG, "→ PUB FAILED [%s] len=%d", QZM_IOTGW_SENSOR_DATA, len);
    return false;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(MQTT_TAG, "Event dispatched from event loop base=%s, event_id=%ld", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(MQTT_TAG, "MQTT connected (broker: %s, client_id: %s)", QZM_IOTGW_BROKER, s_client_id);
            s_mqtt_connected = true;
            if (s_cached_json_len > 0) {
                ESP_LOGI(MQTT_TAG, "  re-publishing cached sensor data (len=%d)", s_cached_json_len);
                esp_mqtt_client_publish(client, QZM_IOTGW_SENSOR_DATA, s_cached_json, s_cached_json_len, 1, 0);
            }
            esp_mqtt_client_publish(client, QZM_IOTGW_STATUS, "{\"status\":\"online\"}",
                                    strlen("{\"status\":\"online\"}"), 1, 1);
            esp_mqtt_client_subscribe(client, QZM_IOTGW_LED_CONTROL, 1);
            ESP_LOGI(MQTT_TAG, "  ← SUB [%s] qos=1", QZM_IOTGW_LED_CONTROL);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(MQTT_TAG, "MQTT disconnected — auto-reconnect in ~2s (keepalive=30s)");
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
        case MQTT_EVENT_DATA: {
            ESP_LOGI(MQTT_TAG, "← DATA [%.*s] len=%d data='%.*s'",
                     event->topic_len, event->topic,
                     event->data_len,
                     (event->data_len > 64) ? 64 : event->data_len,
                     (char*)event->data);

            size_t led_topic_len = strlen(QZM_IOTGW_LED_CONTROL);
            if ((size_t)event->topic_len == led_topic_len &&
                strncmp(event->topic, QZM_IOTGW_LED_CONTROL, led_topic_len) == 0) {
                if (event->data_len >= 1) {
                    char first = ((char*)event->data)[0];
                    if (first == '1') {
                        if (!app_send_command_nonblock("LED_ON", NULL)) {
                            ESP_LOGW(MQTT_TAG, "  → queue full, LED_ON dropped");
                        }
                    } else if (first == '0') {
                        if (!app_send_command_nonblock("LED_OFF", NULL)) {
                            ESP_LOGW(MQTT_TAG, "  → queue full, LED_OFF dropped");
                        }
                    } else {
                        ESP_LOGW(MQTT_TAG, "  → ignore (expected '0' or '1', got '%c')", first);
                    }
                }
            }
            break;
        }
        case MQTT_EVENT_ERROR: {
            esp_mqtt_error_codes_t *err = event->error_handle;
            if (err) {
                ESP_LOGE(MQTT_TAG, "MQTT error — type=%d connect=%d esp_err=%d",
                         err->error_type, err->connect_return_code, err->esp_tls_last_esp_err);
                if (err->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                    ESP_LOGE(MQTT_TAG, "  TCP: stack_err=%d sock_errno=%d",
                             err->esp_tls_stack_err, err->esp_transport_sock_errno);
                }
            } else {
                ESP_LOGE(MQTT_TAG, "MQTT error (no error_handle)");
            }
            break;
        }
        case MQTT_EVENT_BEFORE_CONNECT:
            ESP_LOGD(MQTT_TAG, "MQTT before connect");
            break;
        default:
            ESP_LOGD(MQTT_TAG, "MQTT event %ld", (long)event_id);
            break;
    }
}

static void mqtt_app_start(void)
{
    uint8_t mac[6];
    esp_base_mac_addr_get(mac);

    snprintf(s_client_id, sizeof(s_client_id),
             "qzm_iotgw_%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    const char *lwt_msg = "{\"status\":\"offline\"}";
    const size_t lwt_msg_len = strlen(lwt_msg);

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = QZM_IOTGW_BROKER,
        .session.protocol_ver = MQTT_PROTOCOL_V_5,
        .session.keepalive = 30,
        .session.last_will = {
            .topic = QZM_IOTGW_STATUS,
            .msg = lwt_msg,
            .msg_len = lwt_msg_len,
            .qos = 1,
            .retain = 1,
        },
        .network.reconnect_timeout_ms = 2000,
        .network.disable_auto_reconnect = false,
        .credentials.client_id = s_client_id,
    };
    ESP_LOGI(MQTT_TAG, "Starting MQTT client (client_id: %s, broker: %s)",
             s_client_id, QZM_IOTGW_BROKER);
    s_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(s_mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(s_mqtt_client);
}

static void task_app_mqtt(void *pvParameter)
{
    ESP_LOGI(MQTT_TAG, "MQTT task started");

    // Wait for WiFi using event group (no polling, wakes on connect)
    bool connected = app_wifi_wait_for_connection(pdMS_TO_TICKS(60000));
    if (connected) {
        ESP_LOGI(MQTT_TAG, "WiFi ready, starting MQTT client...");
        mqtt_app_start();
    } else {
        ESP_LOGE(MQTT_TAG, "WiFi not ready within 60s, MQTT client NOT started");
    }

    // Publisher loop: wait for notification from sensor_task, then pull
    // JSON from sensor_data_mgr and publish. 3s timeout keeps the task
    // responsive to the task watchdog (15s timeout in sdkconfig) —
    // timeout does NOT trigger a publish; only notifications do.
    char json_buf[JSON_BUF_SIZE];
    uint32_t pub_cycle = 0;
    esp_task_wdt_add(NULL);
    while (1) {
        BaseType_t got_notify = xTaskNotifyWait(0, 0, NULL, pdMS_TO_TICKS(3000));

        if (got_notify == pdPASS) {
            int json_len = sensor_data_mgr_get_json(json_buf, sizeof(json_buf));
            if (json_len > 0) {
                app_mqtt_publish_data(json_buf, json_len);
                pub_cycle++;
            }
        }
        esp_task_wdt_reset();
    }
}

void task_app_mqtt_start(void)
{
    BaseType_t ret = xTaskCreate(
        task_app_mqtt,
        "task_mqtt",
        8192,
        NULL,
        4,
        &s_mqtt_task_handle
    );

    if (ret == pdPASS) {
        ESP_LOGI(MQTT_TAG, "MQTT task created");
        task_monitor_register("mqtt", s_mqtt_task_handle, 8192);
    } else {
        ESP_LOGE(MQTT_TAG, "Failed to create MQTT task");
    }
}
