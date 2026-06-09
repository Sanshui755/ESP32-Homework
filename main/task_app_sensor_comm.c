#include "task_app_sensor_comm.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "bmp280.h"
#include "sensor_data_mgr.h"
#include <string.h>

static const char *BMP_TAG = "BMP280";

// Task handle
static TaskHandle_t s_bmp_task_handle = NULL;

// BMP280 device descriptor
static bmp280_t s_bmp_dev;

// Scan I2C bus for devices
static void scan_i2c_bus(void)
{
    ESP_LOGI(BMP_TAG, "Scanning I2C bus...");
    
    uint8_t found = 0;
    for (uint8_t addr = 8; addr <= 120; addr++) {
        // Create a temporary device descriptor
        i2c_dev_t dev = {
            .port = 0,
            .addr = addr,
            .cfg = {
                .sda_io_num = GPIO_NUM_6,
                .scl_io_num = GPIO_NUM_7,
                .sda_pullup_en = 1,
                .scl_pullup_en = 1,
                .master = {
                    .clk_speed = 100000
                }
            }
        };
        
        // Check if device is present
        esp_err_t err = i2c_dev_check_present(&dev);
        if (err == ESP_OK) {
            ESP_LOGI(BMP_TAG, "Found device at address 0x%02x", addr);
            found++;
        }
    }
    
    if (found == 0) {
        ESP_LOGW(BMP_TAG, "No I2C devices found!");
    } else {
        ESP_LOGI(BMP_TAG, "Found %d I2C device(s)", found);
    }
}

static void task_app_sensor_comm(void *pvParameter)
{
    ESP_LOGI(BMP_TAG, "BMP280 sensor task started");
    
    // First scan I2C bus to see what's connected
    scan_i2c_bus();
    
    // Initialize BMP280
    bmp280_params_t params;
    bmp280_init_default_params(&params);
    memset(&s_bmp_dev, 0, sizeof(bmp280_t));
    
    // Manually configure I2C device before creating mutex
    s_bmp_dev.i2c_dev.port = 0;
    s_bmp_dev.i2c_dev.addr = BMP280_I2C_ADDRESS_1;  // Try address 0x77 instead of 0x76
    s_bmp_dev.i2c_dev.cfg.sda_io_num = GPIO_NUM_6;
    s_bmp_dev.i2c_dev.cfg.scl_io_num = GPIO_NUM_7;
    s_bmp_dev.i2c_dev.cfg.sda_pullup_en = 1;
    s_bmp_dev.i2c_dev.cfg.scl_pullup_en = 1;
    s_bmp_dev.i2c_dev.cfg.master.clk_speed = 100000;
    
    // Now create the mutex
    esp_err_t ret = i2c_dev_create_mutex(&s_bmp_dev.i2c_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(BMP_TAG, "Failed to create I2C mutex: %s", esp_err_to_name(ret));
        vTaskDelete(NULL);
        return;
    }
    
    ret = bmp280_init(&s_bmp_dev, &params);
    if (ret != ESP_OK) {
        ESP_LOGE(BMP_TAG, "Failed to initialize BMP280: %s", esp_err_to_name(ret));
        vTaskDelete(NULL);
        return;
    }
    
    bool is_bme280 = s_bmp_dev.id == BME280_CHIP_ID;
    ESP_LOGI(BMP_TAG, "Found %s", is_bme280 ? "BME280" : "BMP280");
    
    float pressure, temperature, humidity;
    
    while (1) {
        // Read sensor data
        ret = bmp280_read_float(&s_bmp_dev, &temperature, &pressure, &humidity);
        if (ret != ESP_OK) {
            ESP_LOGE(BMP_TAG, "Failed to read sensor data: %s", esp_err_to_name(ret));
        } else {
            // Log only at DEBUG level
            ESP_LOGD(BMP_TAG, "Temperature: %.2f °C", temperature);
            ESP_LOGD(BMP_TAG, "Pressure: %.2f Pa", pressure);
            if (is_bme280) {
                ESP_LOGD(BMP_TAG, "Humidity: %.2f %%", humidity);
            }
            
            // Update sensor data manager
            sensor_data_mgr_update_temperature(temperature);
            sensor_data_mgr_update_pressure(pressure);
            if (is_bme280) {
                sensor_data_mgr_update_humidity(humidity);
            }
        }
        
        // Wait 2 seconds before next reading
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

void task_app_sensor_comm_start(void)
{
    // Create task with normal priority (priority 3)
    BaseType_t ret = xTaskCreate(
        task_app_sensor_comm,
        "task_app_sensor_comm",
        4096,
        NULL,
        10,  // Normal priority(3)
        &s_bmp_task_handle
    );
    
    if (ret == pdPASS) {
        ESP_LOGI(BMP_TAG, "BMP280 sensor task created successfully");
    } else {
        ESP_LOGE(BMP_TAG, "Failed to create BMP280 sensor task");
    }
}
