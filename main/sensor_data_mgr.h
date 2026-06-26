#ifndef SENSOR_DATA_MGR_H
#define SENSOR_DATA_MGR_H

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// 传感器数据结构体
typedef struct {
    float temperature;       // 温度 (°C) - DHT11
    float humidity;          // 湿度 (%) - DHT11
    int   gas_value;         // 气体传感器值 (ADC) - MICS5524
    uint32_t temperature_ts;
    uint32_t humidity_ts;
    uint32_t gas_ts;
    bool temperature_valid;
    bool humidity_valid;
    bool gas_valid;

    // ICM-20948 9-axis IMU
    float accel_x, accel_y, accel_z;  // m/s²
    float gyro_x,  gyro_y,  gyro_z;  // °/s
    float mag_x,   mag_y,   mag_z;   // µT
    float imu_temp;                    // °C (from ICM20948)
    uint32_t imu_ts;
    bool imu_valid;
    bool mag_valid;                    // magnetometer data valid this cycle
} sensor_data_t;

// 初始化传感器数据管理器
void sensor_data_mgr_init(void);

// 更新 DHT11 温湿度
void sensor_data_mgr_update_temperature(float temp);
void sensor_data_mgr_update_humidity(float humi);

// 更新气体传感器数据
void sensor_data_mgr_update_gas(int gas_value);

// 更新 ICM-20948 9轴数据; mag_valid=false 表示磁力计本周期无数据
void sensor_data_mgr_update_imu(float ax, float ay, float az,
                                float gx, float gy, float gz,
                                float mx, float my, float mz,
                                float t, bool mag_valid);

// 仅更新磁力计（当 Acc/Gyro 读失败但 Mag 可通过 BYPASS 读取时使用）
void sensor_data_mgr_update_mag_only(float mx, float my, float mz);

// 获取最新的传感器数据（拷贝）
bool sensor_data_mgr_get_latest(sensor_data_t *out_data);

// 返回 JSON 字符串长度(>=0), -1=参数错, -2=buffer不足
int sensor_data_mgr_get_json(char *buf, int buf_len);

#endif // SENSOR_DATA_MGR_H
