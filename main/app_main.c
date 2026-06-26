#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include "task_app_cmd.h"
#include "task_app_sensor.h"
#include "task_app_wifi_manage.h"
#include "task_app_mqtt.h"
#include "task_app_btn.h"
#include "sensor_data_mgr.h"
#include "bsp.h"
#include "task_monitor.h"

static const char *TAG = "APP_MAIN";

void app_main(void)
{
    // ---- System init (platform-level) ----
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // ---- Hardware init (BSP: LED, I2C bus) ----
    sensor_data_mgr_init();
    bsp_init();

    // ---- Task monitor ----
    task_monitor_start();

    // ---- Network layer ----
    task_app_wifi_manage_start();

    // ---- Command handler (LED control, etc.) ----
    task_app_cmd_start();
    task_app_mqtt_start();

    // ---- Unified sensor task (DHT11 → MICS5524 → ICM-20948, every 5s) ----
    task_app_sensor_start();

    // ---- Button task (GPIO ISR + debounce → LED toggle) ----
    task_app_btn_start();

    ESP_LOGI(TAG, "All tasks started, app_main returning...");
}
