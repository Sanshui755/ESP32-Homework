/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "task_app_mainbusiness.h"
#include "task_app_sensor_comm.h"
#include "task_app_wifi_manage.h"
#include "task_app_mqtt.h"
#include "i2cdev.h"
#include "sensor_data_mgr.h"
#include "bsp.h"

static const char *TAG = "example";
static const char *MICS5524_TAG = "MICS-5524";
static const char *MONITOR_TAG = "MONITOR";
static const char *BUTTON_TAG = "BUTTON";

#define BUTTON_GPIO 9
#define MICS5524_ADC_CHANNEL ADC_CHANNEL_0

static void mics5524_task(void *pvParameter)
{
    ESP_LOGI(MICS5524_TAG, "MICS-5524 task started");

    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));

    adc_oneshot_chan_cfg_t chan_config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, MICS5524_ADC_CHANNEL, &chan_config));

    int adc_value;
    esp_err_t ret;

    while (1) {
        ret = adc_oneshot_read(adc1_handle, MICS5524_ADC_CHANNEL, &adc_value);
        if (ret == ESP_OK) {
            ESP_LOGD(MICS5524_TAG, "Gas sensor value: %d", adc_value);
            // 更新到传感器数据管理器
            sensor_data_mgr_update_gas(adc_value);
        } else {
            ESP_LOGW(MICS5524_TAG, "Failed to read ADC: %s", esp_err_to_name(ret));
        }

        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }

    ESP_ERROR_CHECK(adc_oneshot_del_unit(adc1_handle));
}

// 任务监控任务 - 低优先级，检测系统状态
static void task_monitor(void *pvParameter)
{
    ESP_LOGI(MONITOR_TAG, "Task monitor started");
    
    while (1) {
        // 当这个任务运行时，说明系统处于 IDLE 状态
        // 因为更高优先级的任务都在等待或延迟

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
static void configure_button(void)
{
    ESP_LOGI(TAG, "Configuring button on GPIO%d", BUTTON_GPIO);
    gpio_reset_pin(BUTTON_GPIO);
    gpio_set_direction(BUTTON_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_GPIO, GPIO_PULLUP_ONLY);
}

static void task_button(void *pvParameter)
{
    ESP_LOGI(BUTTON_TAG, "Button task started");
    
    uint8_t last_button_state = 1;
    uint8_t current_button_state = 1;
    
    while (1) {
        current_button_state = gpio_get_level(BUTTON_GPIO);
        
        if (last_button_state == 0 && current_button_state == 1) {
            // 按键松开，切换LED
            bsp_led_toggle();
            ESP_LOGI(BUTTON_TAG, "Button is pressed");
        }
        
        last_button_state = current_button_state;
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    // Initialize I2C subsystem first
    ESP_ERROR_CHECK(i2cdev_init());
    
    // Initialize sensor data manager
    sensor_data_mgr_init();
    
    // Initialize BSP
    bsp_init();
    
    configure_button();
    
    // 启动监控任务 - 低优先级 1，监控系统状态
    xTaskCreate(task_monitor, "task_monitor", 2048, NULL, 1, NULL);
    
    // Start WiFi management task
    task_app_wifi_manage_start();
    
    // Start MQTT task
    task_app_mqtt_start();
    
    // Start application layer main business task
    task_app_mainbusiness_start();
    
    // Start BMP280 sensor task
    task_app_sensor_comm_start();
    
    // Start DHT11 sensor task (high priority 23)
    task_app_sensor_dht11_start();
    
    // Create sensor tasks
    xTaskCreate(mics5524_task, "mics5524_task", 4096, NULL, 5, NULL);

    // Create button task
    xTaskCreate(task_button, "task_button", 2048, NULL, 5, NULL);

    ESP_LOGI(TAG, "All tasks started, app_main returning...");
}