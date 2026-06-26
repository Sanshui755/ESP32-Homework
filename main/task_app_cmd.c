#include "task_app_cmd.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "bsp.h"
#include "task_monitor.h"
#include <string.h>

static const char *APP_TAG = "APP_CMD";

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
    msg.cmd[sizeof(msg.cmd) - 1] = '\0';
    if (data != NULL) {
        strncpy(msg.data, data, sizeof(msg.data) - 1);
        msg.data[sizeof(msg.data) - 1] = '\0';
    }

    if (xQueueSend(s_cmd_queue, &msg, pdMS_TO_TICKS(100)) == pdPASS) {
        return true;
    } else {
        return false;
    }
}

bool app_send_command_nonblock(const char *cmd, const char *data)
{
    if (s_cmd_queue == NULL || cmd == NULL) {
        return false;
    }

    command_msg_t msg;
    memset(&msg, 0, sizeof(msg));

    strncpy(msg.cmd, cmd, sizeof(msg.cmd) - 1);
    msg.cmd[sizeof(msg.cmd) - 1] = '\0';
    if (data != NULL) {
        strncpy(msg.data, data, sizeof(msg.data) - 1);
        msg.data[sizeof(msg.data) - 1] = '\0';
    }

    return (xQueueSend(s_cmd_queue, &msg, 0) == pdPASS);
}

cmd_id_t cmd_id_from_str(const char *cmd)
{
    if (cmd == NULL) return CMD_ID_NONE;
    if (strcmp(cmd, "LED_ON") == 0) return CMD_ID_LED_ON;
    if (strcmp(cmd, "LED_OFF") == 0) return CMD_ID_LED_OFF;
    return CMD_ID_NONE;
}

typedef enum {
    STATE_WAIT_CMD = 0,
    STATE_LED_ON,
    STATE_LED_OFF,
} task_state_t;

static void task_app_cmd(void *pvParameter)
{
    ESP_LOGI(APP_TAG, "Command handler task started");
    command_msg_t msg;
    task_state_t curr = STATE_WAIT_CMD;
    cmd_id_t pending_cmd = CMD_ID_NONE;

    while (1) {
        switch (curr) {
            case STATE_WAIT_CMD:  /* ── 状态1：等待命令 */
                if (xQueueReceive(s_cmd_queue, &msg, portMAX_DELAY) == pdPASS) {
                    pending_cmd = cmd_id_from_str(msg.cmd);
                    switch (pending_cmd) {
                        case CMD_ID_LED_ON:  curr = STATE_LED_ON;  break;
                        case CMD_ID_LED_OFF: curr = STATE_LED_OFF; break;
                        default:
                            ESP_LOGW(APP_TAG, "Unknown command: %s", msg.cmd);
                            break;
                    }
                }
                break;

            case STATE_LED_ON:      /* ── 状态2：开灯 */
                bsp_led_on();
                curr = STATE_WAIT_CMD;
                break;

            case STATE_LED_OFF:     /* ── 状态3：关灯 */
                bsp_led_off();
                curr = STATE_WAIT_CMD;
                break;
        }
    }
}

void task_app_cmd_start(void)
{
    // Create command queue before starting task to avoid race with MQTT
    s_cmd_queue = xQueueCreate(10, sizeof(command_msg_t));
    if (s_cmd_queue == NULL) {
        ESP_LOGE(APP_TAG, "Failed to create command queue");
        return;
    }

    // Create task with normal priority (priority 3)
    BaseType_t ret = xTaskCreate(
        task_app_cmd,
        "task_app_cmd",
        4096,
        NULL,
        3,
        &s_app_task_handle
    );

    if (ret == pdPASS) {
        ESP_LOGI(APP_TAG, "Command handler task created successfully");
        task_monitor_register("cmd", s_app_task_handle, 4096);
    } else {
        ESP_LOGE(APP_TAG, "Failed to create command handler task");
    }
}
