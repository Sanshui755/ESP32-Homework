#ifndef TASK_APP_BTN_H
#define TASK_APP_BTN_H

/**
 * @brief Initialize and launch the button task (ISR-driven).
 *
 * Configures GPIO for the user button, installs the ISR handler,
 * and starts a FreeRTOS task to handle button events.
 */
void task_app_btn_start(void);

#endif // TASK_APP_BTN_H
