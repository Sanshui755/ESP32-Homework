#ifndef BSP_H
#define BSP_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/i2c_master.h"

// LED 颜色定义
#define BSP_LED_COLOR_RED      16, 0, 0
#define BSP_LED_COLOR_GREEN     0, 16, 0
#define BSP_LED_COLOR_BLUE      0, 0, 16
#define BSP_LED_COLOR_WHITE    16, 16, 16
#define BSP_LED_COLOR_YELLOW    16, 16, 0
#define BSP_LED_COLOR_CYAN      0, 16, 16
#define BSP_LED_COLOR_MAGENTA  16, 0, 16
#define BSP_LED_COLOR_ORANGE   16, 8, 0
#define BSP_LED_COLOR_PINK     16, 0, 8
#define BSP_LED_COLOR_GOLD     16, 8, 0
#define BSP_LED_COLOR_GRAY     8, 8, 8
#define BSP_LED_COLOR_SILVER   8, 8, 8
#define BSP_LED_COLOR_PURPLE   10, 0, 0
#define BSP_LED_COLOR_OFF       0, 0, 0

/**
 * @brief 初始化BSP模块 (LED, I2C总线)
 */
void bsp_init(void);

/**
 * @brief 获取BSP I2C Master总线句柄 (所有I2C传感器共享)
 * @return I2C bus handle, NULL if not initialized
 */
i2c_master_bus_handle_t bsp_i2c_get_bus(void);

/**
 * @brief 设置LED颜色
 */
void bsp_led_set_color(uint8_t red, uint8_t green, uint8_t blue);

/**
 * @brief 关闭LED
 */
void bsp_led_off(void);

/**
 * @brief 打开LED（白色）
 */
void bsp_led_on(void);

/**
 * @brief 切换LED状态
 */
void bsp_led_toggle(uint8_t red, uint8_t green, uint8_t blue);

void bsp_led_on_gold(void);

/**
 * @brief 获取当前LED状态
 * 
 * @return 0-关闭，1-打开
 */
uint8_t bsp_led_get_state(void);

#endif // BSP_H
