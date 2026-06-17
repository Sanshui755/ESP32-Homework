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
static const char *DHT11_TAG = "DHT11";
static const char *MICS5524_TAG = "MICS-5524";
static const char *MONITOR_TAG = "MONITOR";
static const char *BUTTON_TAG = "BUTTON";

#define BUTTON_GPIO 9
#define DHT11_GPIO 4
#define MICS5524_ADC_CHANNEL ADC_CHANNEL_0

typedef struct {
    uint8_t humidity;
    uint8_t temperature;
} dht11_data_t;

static esp_err_t dht11_read(dht11_data_t *data)
{
    if (!data) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t buffer[5] = {0};
    esp_err_t ret = ESP_OK;
    
    // 起始信号
    gpio_set_direction(DHT11_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT11_GPIO, 0);
    esp_rom_delay_us(18000);
    gpio_set_level(DHT11_GPIO, 1);
    
    // ==================== 关键时序开始：禁用中断 ====================
    portDISABLE_INTERRUPTS();
    
    esp_rom_delay_us(40);
    gpio_set_direction(DHT11_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(DHT11_GPIO, GPIO_PULLUP_ONLY);
    
    // 等待DHT11响应
    if (gpio_get_level(DHT11_GPIO) != 0) {
        ret = ESP_ERR_TIMEOUT;
        goto cleanup;
    }
    
    while (gpio_get_level(DHT11_GPIO) == 0);
    while (gpio_get_level(DHT11_GPIO) == 1);
    
    // 读取40位数据
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 8; j++) {
            while (gpio_get_level(DHT11_GPIO) == 0);
            
            int high_time = 0;
            while (gpio_get_level(DHT11_GPIO) == 1 && high_time < 100) {
                esp_rom_delay_us(1);
                high_time++;
            }
            
            buffer[i] <<= 1;
            if (high_time > 40) {
                buffer[i] |= 1;
            }
        }
    }
    
cleanup:
    gpio_set_direction(DHT11_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT11_GPIO, 1);
    
    // ==================== 关键时序结束：启用中断 ====================
    portENABLE_INTERRUPTS();
    
    if (ret != ESP_OK) {
        ESP_LOGW(DHT11_TAG, "DHT11 not responding");
        return ret;
    }
    
    uint8_t check_sum = buffer[0] + buffer[1] + buffer[2] + buffer[3];
    if (check_sum != buffer[4]) {
        ESP_LOGW(DHT11_TAG, "DHT11 checksum error");
        return ESP_ERR_INVALID_CRC;
    }
    
    data->humidity = buffer[0];
    data->temperature = buffer[2];
    
    return ESP_OK;
}

static void dht11_task(void *pvParameter)
{
    ESP_LOGI(DHT11_TAG, "DHT11 task started");

    gpio_reset_pin(DHT11_GPIO);
    gpio_set_direction(DHT11_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT11_GPIO, 1);

    dht11_data_t data;
    esp_err_t ret;

    while (1) {
        // 最多尝试3次
        for (int try = 0; try < 3; try++) {
            ret = dht11_read(&data);
            if (ret == ESP_OK) {
                ESP_LOGD(DHT11_TAG, "Temperature: %d°C, Humidity: %d%%",
                        data.temperature, data.humidity);
                // 更新到传感器数据管理器
                sensor_data_mgr_update_temperature((float)data.temperature);
                sensor_data_mgr_update_humidity((float)data.humidity);
                break;
            } else {
                ESP_LOGW(DHT11_TAG, "Failed to read DHT11 (try %d): %s", try + 1, esp_err_to_name(ret));
                if (try < 2) {
                    // 重试前等待100ms
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                }
            }
        }

        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

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
    
    // Create sensor tasks
    // DHT11 任务设置为最高优先级 23，避免被其他任务抢占
    xTaskCreate(dht11_task, "dht11_task", 4096, NULL, 23, NULL);
    xTaskCreate(mics5524_task, "mics5524_task", 4096, NULL, 5, NULL);

    // Create button task
    xTaskCreate(task_button, "task_button", 2048, NULL, 5, NULL);

    ESP_LOGI(TAG, "All tasks started, app_main returning...");
}