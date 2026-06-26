#ifndef TASK_APP_SENSOR_H
#define TASK_APP_SENSOR_H

/**
 * @brief Launch the unified sensor task.
 *
 * Reads DHT11 → MICS5524 → ICM20948 every 5 seconds,
 * updates sensor_data_mgr, and publishes full JSON via MQTT.
 */
void task_app_sensor_start(void);

#endif // TASK_APP_SENSOR_H
