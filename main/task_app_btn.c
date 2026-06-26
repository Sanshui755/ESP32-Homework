#include "task_app_btn.h"
#include "bsp.h"
#include "task_monitor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <assert.h>

static const char *TAG = "BUTTON";

#define BUTTON_GPIO 9

static QueueHandle_t s_button_queue = NULL;

static void IRAM_ATTR button_isr_handler(void *arg)
{
    (void)arg;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint32_t dummy = 0;
    xQueueSendFromISR(s_button_queue, &dummy, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void button_task(void *pvParameter)
{
    (void)pvParameter;
    ESP_LOGI(TAG, "Button task started (GPIO%d, ISR-driven)", BUTTON_GPIO);

    uint32_t dummy;
    while (1) {
        if (xQueueReceive(s_button_queue, &dummy, portMAX_DELAY) == pdPASS) {
            vTaskDelay(pdMS_TO_TICKS(50));
            int level = gpio_get_level(BUTTON_GPIO);
            if (level == 1) {
                bsp_led_toggle(BSP_LED_COLOR_GOLD);
                ESP_LOGI(TAG, "Button pressed");
            }
        }
    }
}

void task_app_btn_start(void)
{
    s_button_queue = xQueueCreate(5, sizeof(uint32_t));
    assert(s_button_queue != NULL);

    gpio_config_t io_conf = {
        .intr_type    = GPIO_INTR_POSEDGE,
        .mode         = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .pull_up_en   = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&io_conf);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_GPIO, button_isr_handler, NULL);

    TaskHandle_t button_handle;
    BaseType_t ret = xTaskCreate(
        button_task,
        "task_button",
        2048,
        NULL,
        2,
        &button_handle
    );

    if (ret == pdPASS) {
        task_monitor_register("button", button_handle, 2048);
        ESP_LOGI(TAG, "Button task created");
    } else {
        ESP_LOGE(TAG, "Failed to create button task");
    }
}
