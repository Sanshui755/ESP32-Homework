idf_component_register(SRCS "app_main.c" "task_monitor.c" "task_app_cmd.c" "task_app_sensor.c" "task_app_wifi_manage.c" "task_app_mqtt.c" "task_app_btn.c" "sensor_data_mgr.c" "bsp.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES esp_adc driver led_strip esp_wifi esp_event nvs_flash mqtt)
