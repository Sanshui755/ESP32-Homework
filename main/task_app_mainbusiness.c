#include "task_app_mainbusiness.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "sensor_data_mgr.h"
#include "bsp.h"
#include <string.h>

static const char *APP_TAG = "APP_MAIN";

// Sensor data storage (from sensor_data_mgr)
static sensor_data_t s_sensor_data = {0};

// State machine variables
static app_state_t s_current_state = APP_STATE_INIT;
static app_state_t s_next_state = APP_STATE_INIT;

// Message queue handle
static QueueHandle_t s_cmd_queue = NULL;

// Task handle
static TaskHandle_t s_app_task_handle = NULL;

bool app_send_command(const char *cmd, const char *data)
{
    if (s_cmd_queue == NULL || cmd == NULL) {
        return false;
    }
    
    command_msg_t msg;
    memset(&msg, 0, sizeof(msg));
    
    strncpy(msg.cmd, cmd, sizeof(msg.cmd) - 1);
    if (data != NULL) {
        strncpy(msg.data, data, sizeof(msg.data) - 1);
    }
    
    if (xQueueSend(s_cmd_queue, &msg, pdMS_TO_TICKS(100)) == pdPASS) {
        return true;
    } else {
        return false;
    }
}

static void handle_state_init(void)
{
    // Initialize message queue (capacity 10)
    s_cmd_queue = xQueueCreate(10, sizeof(command_msg_t));
    if (s_cmd_queue == NULL) {
        ESP_LOGE(APP_TAG, "Failed to create command queue");
        return;
    }
    s_next_state = APP_STATE_WAIT_CMD;
}

static void handle_state_wait_cmd(void)
{
    command_msg_t msg;
    
    // Wait for command with 100ms timeout
    if (xQueueReceive(s_cmd_queue, &msg, pdMS_TO_TICKS(100)) == pdPASS) {
        if (strcmp(msg.cmd, "LED_ON") == 0) {
            s_next_state = APP_STATE_LED_ON;
        } else if (strcmp(msg.cmd, "LED_OFF") == 0) {
            s_next_state = APP_STATE_LED_OFF;
        }
    }
}

static void handle_state_led_on(void)
{
    bsp_led_on();
    s_next_state = APP_STATE_WAIT_CMD;
}

static void handle_state_led_off(void)
{
    bsp_led_off();
    s_next_state = APP_STATE_WAIT_CMD;
}

static void update_sensor_data(void)
{
    // Get latest sensor data (no logging - MQTT takes care of reporting)
    sensor_data_mgr_get_latest(&s_sensor_data);
}

static void process_state_machine(void)
{
    // Update sensor data first
    update_sensor_data();
    
    // Execute current state
    switch (s_current_state) {
        case APP_STATE_INIT:
            handle_state_init();
            break;
            
        case APP_STATE_WAIT_CMD:
            handle_state_wait_cmd();
            break;
            
        case APP_STATE_LED_ON:
            handle_state_led_on();
            bsp_led_set_color(BSP_LED_COLOR_BLUE);
            break;
            
        case APP_STATE_LED_OFF:
            handle_state_led_off();
            bsp_led_set_color(BSP_LED_COLOR_OFF);
            break;
            
        case APP_STATE_END:
            // S100: End state
            break;
            
        default:
            break;
    }
    
    // State transition
    if (s_next_state != s_current_state) {
        s_current_state = s_next_state;
    }
}

static void task_app_mainbusiness(void *pvParameter)
{
    ESP_LOGI(APP_TAG, "Main business task started");
    
    while (1) {
        process_state_machine();
        vTaskDelay(10 / portTICK_PERIOD_MS);  // 10ms延迟，让出CPU
    }
}

void task_app_mainbusiness_start(void)
{
    // Create task with normal priority (priority 3)
    BaseType_t ret = xTaskCreate(
        task_app_mainbusiness,
        "task_app_mainbusiness",
        4096,
        NULL,
        3,  // Normal priority
        &s_app_task_handle
    );
    
    if (ret == pdPASS) {
        ESP_LOGI(APP_TAG, "Main business task created successfully");
    } else {
        ESP_LOGE(APP_TAG, "Failed to create main business task");
    }
}
