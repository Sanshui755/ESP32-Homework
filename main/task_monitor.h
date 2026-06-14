#ifndef TASK_MONITOR_H
#define TASK_MONITOR_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * @brief Register a task for stack monitoring
 *
 * Call this from each module after task creation succeeds.
 * Max 16 tasks can be registered.
 *
 * @param name         Display name (static string, not copied)
 * @param handle       Task handle from xTaskCreate
 * @param stack_total  Total stack size in BYTES (the value passed as
 *                     the usStackDepth parameter to xTaskCreate, which
 *                     is also bytes in ESP-IDF).  Internally, the
 *                     high-water-mark value (which is in StackType_t
 *                     words) is converted to bytes before comparison.
 */
void task_monitor_register(const char *name, TaskHandle_t handle, uint32_t stack_total);

/**
 * @brief Unregister a task from monitoring
 *
 * Call this before a task deletes itself so the monitor will
 * not attempt to read its stack high-water mark.
 *
 * @param handle Task handle from xTaskCreate
 */
void task_monitor_unregister(TaskHandle_t handle);

/**
 * @brief Initialize and start the task monitor
 *
 * Creates a low-priority (1) monitoring task that periodically
 * reports free heap and stack high-water marks of all registered tasks.
 */
void task_monitor_start(void);

#endif // TASK_MONITOR_H
