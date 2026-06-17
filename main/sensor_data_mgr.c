#include "sensor_data_mgr.h"
#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "freertos/task.h"

static const char *TAG = "SensorDataMgr";

// 全局传感器数据
static sensor_data_t s_sensor_data;
static SemaphoreHandle_t s_data_mutex = NULL;

void sensor_data_mgr_init(void)
{
    memset(&s_sensor_data, 0, sizeof(s_sensor_data));
    s_data_mutex = xSemaphoreCreateMutex();
    ESP_LOGI(TAG, "Sensor data manager initialized");
}

static void lock_data(void)
{
    if (s_data_mutex) {
        xSemaphoreTake(s_data_mutex, portMAX_DELAY);
    }
}

static void unlock_data(void)
{
    if (s_data_mutex) {
        xSemaphoreGive(s_data_mutex);
    }
}

void sensor_data_mgr_update_temperature(float temp)
{
    lock_data();
    s_sensor_data.temperature = temp;
    s_sensor_data.temperature_valid = true;
    s_sensor_data.timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;
    unlock_data();
}

void sensor_data_mgr_update_humidity(float humi)
{
    lock_data();
    s_sensor_data.humidity = humi;
    s_sensor_data.humidity_valid = true;
    s_sensor_data.timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;
    unlock_data();
}

void sensor_data_mgr_update_pressure(float press)
{
    lock_data();
    s_sensor_data.pressure = press;
    s_sensor_data.pressure_valid = true;
    s_sensor_data.timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;
    unlock_data();
}

void sensor_data_mgr_update_gas(int gas_value)
{
    lock_data();
    s_sensor_data.gas_value = gas_value;
    s_sensor_data.gas_valid = true;
    s_sensor_data.timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;
    unlock_data();
}

bool sensor_data_mgr_get_latest(sensor_data_t *out_data)
{
    if (!out_data) {
        return false;
    }
    
    lock_data();
    memcpy(out_data, &s_sensor_data, sizeof(sensor_data_t));
    unlock_data();
    
    return true;
}

int sensor_data_mgr_get_json(char *buf, int buf_len)
{
    if (!buf || buf_len <= 0) {
        return -1;
    }
    
    sensor_data_t data;
    sensor_data_mgr_get_latest(&data);
    
    int len = snprintf(buf, buf_len,
        "{"
        "\"temperature\":%.2f,"
        "\"humidity\":%.2f,"
        "\"pressure\":%.2f,"
        "\"gas\":%d,"
        "\"temp_valid\":%s,"
        "\"humi_valid\":%s,"
        "\"press_valid\":%s,"
        "\"gas_valid\":%s,"
        "\"timestamp\":%lu"
        "}",
        data.temperature,
        data.humidity,
        data.pressure,
        data.gas_value,
        data.temperature_valid ? "true" : "false",
        data.humidity_valid ? "true" : "false",
        data.pressure_valid ? "true" : "false",
        data.gas_valid ? "true" : "false",
        (unsigned long)data.timestamp
    );
    
    return len;
}
