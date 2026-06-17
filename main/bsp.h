#ifndef BSP_H
#define BSP_H

#include <stdint.h>
#include "esp_err.h"

// LED 颜色定义
#define BSP_LED_COLOR_RED      16, 0, 0
#define BSP_LED_COLOR_GREEN     0, 16, 0
#define BSP_LED_COLOR_BLUE      0, 0, 16
#define BSP_LED_COLOR_WHITE    16, 16, 16
#define BSP_LED_COLOR_YELLOW    16, 16, 0
#define BSP_LED_COLOR_OFF       0, 0, 0

/**
 * @brief 初始化BSP模块
 */
void bsp_init(void);

/**
 * @brief 设置LED颜色
 * 
 * @param red 红色分量 (0-255)
 * @param green 绿色分量 (0-255)
 * @param blue 蓝色分量 (0-255)
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
void bsp_led_toggle(void);

/**
 * @brief 获取当前LED状态
 * 
 * @return 0-关闭，1-打开
 */
uint8_t bsp_led_get_state(void);

#endif // BSP_H
