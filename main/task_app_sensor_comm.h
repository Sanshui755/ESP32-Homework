#ifndef TASK_APP_SENSOR_COMM_H
#define TASK_APP_SENSOR_COMM_H

/**
 * @brief Initialize and start the BMP280 sensor task
 * 
 * This function creates the BMP280 sensor task with normal priority.
 * The task reads temperature and pressure from BMP280 every 2 seconds
 * and logs the values.
 */
void task_app_sensor_comm_start(void);

#endif // TASK_APP_SENSOR_COMM_H
