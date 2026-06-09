#ifndef TASK_APP_WIFI_MANAGE_H
#define TASK_APP_WIFI_MANAGE_H

#include <stdint.h>
#include <stdbool.h>

// WiFi 连接状态定义
typedef enum {
    WIFI_STATE_DISCONNECTED = 0,   // 未连接
    WIFI_STATE_CONNECTING,         // 正在连接
    WIFI_STATE_CONNECTED,          // 已连接
    WIFI_STATE_ERROR               // 错误状态
} wifi_state_t;

/**
 * @brief Get current WiFi state
 * 
 * @return Current WiFi state
 */
wifi_state_t app_get_wifi_state(void);

/**
 * @brief Check if WiFi is connected
 * 
 * @return true if connected, false otherwise
 */
bool app_wifi_is_connected(void);

/**
 * @brief Initialize and start WiFi management task
 */
void task_app_wifi_manage_start(void);

#endif // TASK_APP_WIFI_MANAGE_H
