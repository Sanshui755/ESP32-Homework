idf_component_register(SRCS "blink_example_main.c" "task_app_mainbusiness.c" "task_app_sensor_comm.c" "task_app_wifi_manage.c" "task_app_mqtt.c" "sensor_data_mgr.c" "bsp.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES esp_adc driver led_strip bmp280 esp_wifi_remote esp_wifi esp_event nvs_flash mqtt)
