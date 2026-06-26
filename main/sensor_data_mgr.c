#include "sensor_data_mgr.h"
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include "esp_log.h"
#include "freertos/task.h"

static const char *TAG = "SensorDataMgr";

#define STALE_THRESHOLD_MS 30000

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
    if (s_data_mutex) xSemaphoreTake(s_data_mutex, portMAX_DELAY);
}

static void unlock_data(void)
{
    if (s_data_mutex) xSemaphoreGive(s_data_mutex);
}

void sensor_data_mgr_update_temperature(float temp)
{
    lock_data();
    s_sensor_data.temperature = temp;
    s_sensor_data.temperature_valid = true;
    s_sensor_data.temperature_ts = xTaskGetTickCount() * portTICK_PERIOD_MS;
    unlock_data();
}

void sensor_data_mgr_update_humidity(float humi)
{
    lock_data();
    s_sensor_data.humidity = humi;
    s_sensor_data.humidity_valid = true;
    s_sensor_data.humidity_ts = xTaskGetTickCount() * portTICK_PERIOD_MS;
    unlock_data();
}

void sensor_data_mgr_update_gas(int gas_value)
{
    lock_data();
    s_sensor_data.gas_value = gas_value;
    s_sensor_data.gas_valid = true;
    s_sensor_data.gas_ts = xTaskGetTickCount() * portTICK_PERIOD_MS;
    unlock_data();
}

void sensor_data_mgr_update_imu(float ax, float ay, float az,
                                float gx, float gy, float gz,
                                float mx, float my, float mz,
                                float t, bool mag_valid)
{
    lock_data();
    s_sensor_data.accel_x = ax; s_sensor_data.accel_y = ay; s_sensor_data.accel_z = az;
    s_sensor_data.gyro_x  = gx; s_sensor_data.gyro_y  = gy; s_sensor_data.gyro_z  = gz;
    s_sensor_data.mag_x   = mx; s_sensor_data.mag_y   = my; s_sensor_data.mag_z   = mz;
    s_sensor_data.mag_valid = mag_valid;
    s_sensor_data.imu_temp = t;
    s_sensor_data.imu_valid = true;
    s_sensor_data.imu_ts = xTaskGetTickCount() * portTICK_PERIOD_MS;
    unlock_data();
}

void sensor_data_mgr_update_mag_only(float mx, float my, float mz)
{
    lock_data();
    s_sensor_data.mag_x = mx;
    s_sensor_data.mag_y = my;
    s_sensor_data.mag_z = mz;
    s_sensor_data.mag_valid = true;
    // don't touch accel/gyro/imu_temp/imu_valid — they keep the last known good values
    unlock_data();
}

bool sensor_data_mgr_get_latest(sensor_data_t *out_data)
{
    if (!out_data) return false;
    lock_data();
    memcpy(out_data, &s_sensor_data, sizeof(sensor_data_t));
    unlock_data();
    return true;
}

int sensor_data_mgr_get_json(char *buf, int buf_len)
{
    if (!buf || buf_len <= 0) return -1;

    sensor_data_t data;
    sensor_data_mgr_get_latest(&data);

    uint32_t now_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
    bool temp_fresh  = (data.temperature_valid && (now_ms - data.temperature_ts < STALE_THRESHOLD_MS));
    bool humi_fresh  = (data.humidity_valid && (now_ms - data.humidity_ts    < STALE_THRESHOLD_MS));
    bool gas_fresh   = (data.gas_valid        && (now_ms - data.gas_ts         < STALE_THRESHOLD_MS));
    bool imu_fresh    = (data.imu_valid      && (now_ms - data.imu_ts            < STALE_THRESHOLD_MS));

    int pos = 0, needed;
    needed = snprintf(buf + pos, buf_len - pos, "{");
    if (needed < 0) return -1;
    pos += needed;
    if (pos >= buf_len - 1) return -2;

    bool first = true;

    if (temp_fresh) {
        needed = snprintf(buf + pos, buf_len - pos,
                          "%s\"temperature\":%.2f,\"temperature_ts\":%" PRIu32,
                          first ? "" : ",", data.temperature, data.temperature_ts);
        if (needed < 0) return -1;
        pos += needed; first = false;
        if (pos >= buf_len - 1) return -2;
    }
    if (humi_fresh) {
        needed = snprintf(buf + pos, buf_len - pos,
                          "%s\"humidity\":%.2f,\"humidity_ts\":%" PRIu32,
                          first ? "" : ",", data.humidity, data.humidity_ts);
        if (needed < 0) return -1;
        pos += needed; first = false;
        if (pos >= buf_len - 1) return -2;
    }
    if (gas_fresh) {
        needed = snprintf(buf + pos, buf_len - pos,
                          "%s\"gas\":%d,\"gas_ts\":%" PRIu32,
                          first ? "" : ",", data.gas_value, data.gas_ts);
        if (needed < 0) return -1;
        pos += needed; first = false;
        if (pos >= buf_len - 1) return -2;
    }
    if (imu_fresh) {
        needed = snprintf(buf + pos, buf_len - pos,
                          "%s\"imu\":{\"ax\":%.3f,\"ay\":%.3f,\"az\":%.3f,"
                          "\"gx\":%.2f,\"gy\":%.2f,\"gz\":%.2f,"
                          "\"t\":%.1f,\"ts\":%" PRIu32,
                          first ? "" : ",", data.accel_x, data.accel_y, data.accel_z,
                          data.gyro_x, data.gyro_y, data.gyro_z,
                          data.imu_temp, data.imu_ts);
        if (needed < 0) return -1;
        pos += needed;
        if (pos >= buf_len - 1) return -2;
        if (data.mag_valid) {
            needed = snprintf(buf + pos, buf_len - pos,
                              ",\"mx\":%.1f,\"my\":%.1f,\"mz\":%.1f",
                              data.mag_x, data.mag_y, data.mag_z);
            if (needed < 0) return -1;
            pos += needed;
            if (pos >= buf_len - 1) return -2;
        }
        needed = snprintf(buf + pos, buf_len - pos, "}");
        if (needed < 0) return -1;
        pos += needed;
        if (pos >= buf_len - 1) return -2;
        first = false;
    }

    if (pos >= buf_len - 1) return -2;
    buf[pos++] = '}';
    buf[pos] = '\0';
    return pos;
}
