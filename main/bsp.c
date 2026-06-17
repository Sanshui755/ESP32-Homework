#include "bsp.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"

static const char *BSP_TAG = "BSP";

#define BSP_BLINK_GPIO CONFIG_BLINK_GPIO

static uint8_t s_led_state = 0;
static led_strip_handle_t s_led_strip_handle = NULL;

void bsp_init(void)
{
    ESP_LOGI(BSP_TAG, "Initializing BSP module");
    
    // 配置LED
    led_strip_config_t strip_config = {
        .strip_gpio_num = BSP_BLINK_GPIO,
        .max_leds = 1,
    };

#if CONFIG_BLINK_LED_STRIP_BACKEND_RMT
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000,
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &s_led_strip_handle));
#elif CONFIG_BLINK_LED_STRIP_BACKEND_SPI
    led_strip_spi_config_t spi_config = {
        .spi_bus = SPI2_HOST,
        .flags.with_dma = true,
    };
    ESP_ERROR_CHECK(led_strip_new_spi_device(&strip_config, &spi_config, &s_led_strip_handle));
#else
#error "unsupported LED strip backend"
#endif
    
    // 初始关闭LED
    led_strip_clear(s_led_strip_handle);
}

void bsp_led_set_color(uint8_t red, uint8_t green, uint8_t blue)
{
    if (s_led_strip_handle) {
        led_strip_set_pixel(s_led_strip_handle, 0, red, green, blue);
        led_strip_refresh(s_led_strip_handle);
        // 记录状态：如果不是全黑，则视为打开
        s_led_state = (red | green | blue) ? 1 : 0;
    }
}

void bsp_led_off(void)
{
    if (s_led_strip_handle) {
        led_strip_clear(s_led_strip_handle);
        s_led_state = 0;
    }
}

void bsp_led_on(void)
{
    bsp_led_set_color(BSP_LED_COLOR_WHITE);
}

void bsp_led_toggle(void)
{
    if (s_led_state) {
        bsp_led_off();
    } else {
        bsp_led_on();
    }
}

uint8_t bsp_led_get_state(void)
{
    return s_led_state;
}
