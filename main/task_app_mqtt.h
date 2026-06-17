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

#endif // TASK_APP_MQTT_H
