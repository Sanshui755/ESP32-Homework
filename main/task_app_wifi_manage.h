#ifndef TASK_APP_WIFI_MANAGE_H
#define TASK_APP_WIFI_MANAGE_H

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// WiFi 连接状态定义
typedef enum {
    WIFI_STATE_DISCONNECTED = 0,   // 未连接
    WIFI_STATE_CONNECTING,         // 正在连接
    WIFI_STATE_CONNECTED,          // 已连接
    WIFI_STATE_ERROR               // 错误状态
} wifi_state_t;

/**
 * @brief Get current WiFi state
 */
wifi_state_t app_get_wifi_state(void);

/**
 * @brief Check if WiFi is connected
 */
bool app_wifi_is_connected(void);

/**
 * @brief Block the calling task until WiFi is connected or timeout.
 *        Uses FreeRTOS event group (no polling).
 * @param timeout_ticks Maximum time to wait, in FreeRTOS ticks
 * @return true if connected, false if timeout
 */
bool app_wifi_wait_for_connection(TickType_t timeout_ticks);

/**
 * @brief Initialize and start WiFi management task
 */
void task_app_wifi_manage_start(void);

#endif // TASK_APP_WIFI_MANAGE_H
