#ifndef TASK_APP_MAINBUSINESS_H
#define TASK_APP_MAINBUSINESS_H

#include <stdint.h>
#include <stdbool.h>

// 温度告警阈值
#define ALARM_TEMP_THRESHOLD  32.0f

// 状态机状态定义
typedef enum {
    APP_STATE_INIT = 0,        // S0: 初始状态
    APP_STATE_WAIT_CMD,        // S1: 等待命令状态
    APP_STATE_LED_ON,          // S2: 开灯状态
    APP_STATE_LED_OFF,         // S3: 关灯状态
    APP_STATE_END = 100        // S100: 结束状态
} app_state_t;

// 命令消息结构体
typedef struct {
    char cmd[20];     // 命令，20字节
    char data[30];    // 数据，30字节
} command_msg_t;

/**
 * @brief 发送命令到主业务任务
 * 
 * @param cmd 命令字符串
 * @param data 数据字符串
 * @return 成功返回true，失败返回false
 */
bool app_send_command(const char *cmd, const char *data);

/**
 * @brief Initialize and start the main business task
 * 
 * This function creates the main business task with normal priority.
 * The task handles the core application logic including sensor data
 * aggregation and system coordination.
 */
void task_app_mainbusiness_start(void);

#endif // TASK_APP_MAINBUSINESS_H