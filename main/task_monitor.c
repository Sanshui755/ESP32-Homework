#include "task_monitor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include <string.h>

static const char *MONITOR_TAG = "MONITOR";

#define MONITOR_INTERVAL_MS 30000
#define MAX_MONITORED_TASKS 16

typedef struct {
    const char *name;
    TaskHandle_t handle;
    uint32_t stack_total;
} monitored_task_t;

static monitored_task_t s_tasks[MAX_MONITORED_TASKS];
static int s_task_count = 0;

void task_monitor_register(const char *name, TaskHandle_t handle, uint32_t stack_total)
{
    if (s_task_count >= MAX_MONITORED_TASKS || !name || !handle || stack_total == 0) return;

    /* NOTE: stack_total is in bytes (as documented in the header).
     * uxTaskGetStackHighWaterMark returns StackType_t WORDS, so we
     * convert stack_total to words and back for the calculations in
     * the reporting loop (not here). */
    s_tasks[s_task_count].name = name;
    s_tasks[s_task_count].handle = handle;
    s_tasks[s_task_count].stack_total = stack_total;
    s_task_count++;
    ESP_LOGI(MONITOR_TAG, "Registered task: %s (stack=%u bytes)", name, (unsigned)stack_total);
}

void task_monitor_unregister(TaskHandle_t handle)
{
    for (int i = 0; i < s_task_count; i++) {
        if (s_tasks[i].handle == handle) {
            ESP_LOGI(MONITOR_TAG, "Unregistered task: %s", s_tasks[i].name);
            // Move last entry into this slot to keep array compact
            s_tasks[i] = s_tasks[s_task_count - 1];
            s_task_count--;
            return;
        }
    }
}

static void task_monitor(void *pvParameter)
{
    // Wait for app_main to finish registering all tasks before first report
    vTaskDelay(pdMS_TO_TICKS(5000));

    ESP_LOGI(MONITOR_TAG, "Task monitor active, %d tasks registered", s_task_count);

    while (1) {
        size_t free_heap = heap_caps_get_free_size(MALLOC_CAP_8BIT);
        size_t min_heap = heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT);
        ESP_LOGI(MONITOR_TAG,
                 "Free heap: %u bytes (%u KB), min-ever: %u bytes (%u KB) — %d tasks",
                 (unsigned)free_heap, (unsigned)(free_heap / 1024),
                 (unsigned)min_heap, (unsigned)(min_heap / 1024),
                 s_task_count);

        for (int i = 0; i < s_task_count; i++) {
            /* uxTaskGetStackHighWaterMark returns the LOWEST value
             * that the stack free space has ever reached — measured
             * in StackType_t WORDS, NOT bytes.  Multiply by
             * sizeof(StackType_t) (== 4 on ESP32-C3) to get bytes. */
            UBaseType_t free_words = uxTaskGetStackHighWaterMark(s_tasks[i].handle);
            uint32_t stack_total   = s_tasks[i].stack_total;
            uint32_t min_free_bytes = (uint32_t)free_words * sizeof(StackType_t);

            /* Peak stack usage in bytes.  Saturate to stack_total to
             * avoid wrap-around in the (impossible) case where
             * free_words > stack_total / sizeof(StackType_t). */
            uint32_t used_peak_bytes = (min_free_bytes < stack_total)
                                        ? (stack_total - min_free_bytes)
                                        : stack_total;

            unsigned percent = (stack_total > 0)
                                 ? (unsigned)(used_peak_bytes * 100u / stack_total)
                                 : 0;

            ESP_LOGI(MONITOR_TAG,
                     "  %-24s stack_total=%uB  min_free=%uB  used_peak=%uB  (%u%%)",
                     s_tasks[i].name,
                     (unsigned)stack_total,
                     (unsigned)min_free_bytes,
                     (unsigned)used_peak_bytes,
                     percent);
        }

        vTaskDelay(pdMS_TO_TICKS(MONITOR_INTERVAL_MS));
    }
}

void task_monitor_start(void)
{
    memset(s_tasks, 0, sizeof(s_tasks));
    s_task_count = 0;

    BaseType_t ret = xTaskCreate(
        task_monitor,
        "task_monitor",
        2048,
        NULL,
        1,
        NULL
    );

    if (ret == pdPASS) {
        ESP_LOGI(MONITOR_TAG, "Task monitor created successfully");
    } else {
        ESP_LOGE(MONITOR_TAG, "Failed to create task monitor");
    }
}
