#include "bsp.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "led_strip.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "sdkconfig.h"

static const char *BSP_TAG = "BSP";

#define BSP_BLINK_GPIO CONFIG_BLINK_GPIO

// I2C bus (shared by all I2C sensors: ICM20948, etc.)
#define I2C_SDA_GPIO  GPIO_NUM_6
#define I2C_SCL_GPIO  GPIO_NUM_7
#define I2C_CLK_SPEED 100000

static i2c_master_bus_handle_t s_i2c_bus = NULL;

static uint8_t s_led_state = 0;
#if CONFIG_BLINK_LED_STRIP
static led_strip_handle_t s_led_strip_handle = NULL;
#endif

void bsp_init(void)
{
    ESP_LOGI(BSP_TAG, "Initializing BSP module");

    // ── I2C master bus ──
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port           = -1,
        .sda_io_num         = I2C_SDA_GPIO,
        .scl_io_num         = I2C_SCL_GPIO,
        .clk_source         = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt  = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &s_i2c_bus));
    ESP_LOGI(BSP_TAG, "I2C bus created (SDA=%d, SCL=%d, %dHz)", I2C_SDA_GPIO, I2C_SCL_GPIO, I2C_CLK_SPEED);

    // ── LED ──
#if CONFIG_BLINK_LED_STRIP
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

    led_strip_clear(s_led_strip_handle);
#else
    gpio_reset_pin(BSP_BLINK_GPIO);
    gpio_set_direction(BSP_BLINK_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(BSP_BLINK_GPIO, 0);
    ESP_LOGI(BSP_TAG, "LED initialized on GPIO%d (GPIO mode)", BSP_BLINK_GPIO);
#endif
}

i2c_master_bus_handle_t bsp_i2c_get_bus(void)
{
    return s_i2c_bus;
}

void bsp_led_set_color(uint8_t red, uint8_t green, uint8_t blue)
{
#if CONFIG_BLINK_LED_STRIP
    if (s_led_strip_handle) {
        led_strip_set_pixel(s_led_strip_handle, 0, red, green, blue);
        led_strip_refresh(s_led_strip_handle);
        s_led_state = (red | green | blue) ? 1 : 0;
    }
#else
    s_led_state = (red | green | blue) ? 1 : 0;
    gpio_set_level(BSP_BLINK_GPIO, s_led_state);
#endif
}

void bsp_led_off(void)
{
#if CONFIG_BLINK_LED_STRIP
    if (s_led_strip_handle) {
        led_strip_clear(s_led_strip_handle);
    }
#endif
    gpio_set_level(BSP_BLINK_GPIO, 0);
    s_led_state = 0;
}

void bsp_led_on(void)
{
    bsp_led_set_color(BSP_LED_COLOR_BLUE);
}

void bsp_led_on_red(void)
{
    bsp_led_set_color(BSP_LED_COLOR_RED);
}

void bsp_led_on_gold(void)
{
    bsp_led_set_color(BSP_LED_COLOR_GOLD);

}

void bsp_led_on_green(void)
{
    bsp_led_set_color(BSP_LED_COLOR_GREEN);
}

void bsp_led_on_blue(void)
{
    bsp_led_set_color(BSP_LED_COLOR_BLUE);
}

void bsp_led_toggle(uint8_t red, uint8_t green, uint8_t blue)
{
    if (s_led_state) {
        bsp_led_off();
    } else {
        bsp_led_set_color(red, green, blue);
    }
}

uint8_t bsp_led_get_state(void)
{
    return s_led_state;
}
