#ifndef TASK_APP_MQTT_H
#define TASK_APP_MQTT_H

#include <stdbool.h>

/**
 * @brief 初始化并启动 MQTT 任务
 */
void task_app_mqtt_start(void);

/**
 * @brief 获取 MQTT 连接状态
 */
bool app_mqtt_is_connected(void);

/**
 * @brief Publish sensor data JSON to the MQTT data topic.
 *        Updates cached JSON for reconnect re-publish.
 * @return true on success, false if not connected or publish failed
 */
bool app_mqtt_publish_data(const char *json, int len);

/**
 * @brief Notify MQTT task that fresh sensor data is available.
 *        MQTT task will then pull JSON from sensor_data_mgr and publish.
 *        Safe to call from any task (including ISR-free sensor tasks).
 */
void app_mqtt_notify_new_data(void);

#endif // TASK_APP_MQTT_H
