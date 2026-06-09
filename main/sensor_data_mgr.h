#ifndef SENSOR_DATA_MGR_H
#define SENSOR_DATA_MGR_H

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// 传感器数据结构体
typedef struct {
    float temperature;      // 温度 (°C)
    float humidity;         // 湿度 (%)
    float pressure;         // 气压 (Pa)
    int gas_value;          // 气体传感器值 (ADC)
    uint32_t timestamp;     // 时间戳 (ms)
    bool temperature_valid; // 温度数据是否有效
    bool humidity_valid;    // 湿度数据是否有效
    bool pressure_valid;    // 气压数据是否有效
    bool gas_valid;         // 气体数据是否有效
} sensor_data_t;

// 初始化传感器数据管理器
void sensor_data_mgr_init(void);

// 更新温度数据
void sensor_data_mgr_update_temperature(float temp);

// 更新湿度数据
void sensor_data_mgr_update_humidity(float humi);

// 更新气压数据
void sensor_data_mgr_update_pressure(float press);

// 更新气体传感器数据
void sensor_data_mgr_update_gas(int gas_value);

// 获取最新的传感器数据（拷贝）
// 返回值：是否成功获取
bool sensor_data_mgr_get_latest(sensor_data_t *out_data);

// 获取传感器数据 JSON 字符串
// 返回值：JSON 字符串长度，负数表示失败
int sensor_data_mgr_get_json(char *buf, int buf_len);

#endif // SENSOR_DATA_MGR_H
