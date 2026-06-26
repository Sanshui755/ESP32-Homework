#ifndef TASK_APP_CMD_H
#define TASK_APP_CMD_H

#include <stdint.h>
#include <stdbool.h>

// 命令消息结构体
typedef struct {
    char cmd[20];     // 命令，20字节
    char data[30];    // 数据，30字节
} command_msg_t;

// 命令 ID（用于 switch 分发）
typedef enum {
    CMD_ID_NONE = 0,
    CMD_ID_LED_ON,
    CMD_ID_LED_OFF,
} cmd_id_t;

// 命令字符串 → 命令 ID
cmd_id_t cmd_id_from_str(const char *cmd);

/**
 * @brief 发送命令到命令处理任务
 *
 * @param cmd 命令字符串
 * @param data 数据字符串
 * @return 成功返回true，失败返回false
 */
bool app_send_command(const char *cmd, const char *data);

/**
 * @brief Non-blocking version of app_send_command(). Safe to call from
 *        event loop / interrupt context. Returns immediately if queue full.
 *
 * @param cmd  Command string
 * @param data Data string (may be NULL)
 * @return true if queued successfully, false if queue full or invalid input
 */
bool app_send_command_nonblock(const char *cmd, const char *data);

/**
 * @brief Initialize and start the command handler task
 *
 * This function creates the command handler task with normal priority.
 * The task receives commands from other modules (MQTT, button, etc.)
 * via a FreeRTOS queue and executes them through a state machine.
 */
void task_app_cmd_start(void);

#endif // TASK_APP_CMD_H
