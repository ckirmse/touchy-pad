// SPDX-License-Identifier: GPL-3.0-or-later

#include "board.h"
#include "board_pins.h"
#include "platform.h"

#include "esp_log.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/ledc.h"

static const char *TAG = "board";

static i2c_master_bus_handle_t s_i2c_bus = nullptr;

i2c_master_bus_handle_t board_get_i2c_bus(void) { return s_i2c_bus; }

extern "C" void board_backlight_set(bool on)
{
    // 11-bit LEDC (0-2047). Full on or full off.
    uint32_t duty = on ? 2047 : 0;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

extern "C" void board_init(void)
{
    // Backlight boost power rail: always HIGH.
    gpio_set_direction(BOARD_BK_PWR_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(BOARD_BK_PWR_GPIO, 1);

    // Hold ESP32-C6 companion in reset (WiFi not used; prevents SDIO lines floating).
    gpio_set_direction(BOARD_C6_RESET_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(BOARD_C6_RESET_GPIO, 0);

    // I2C bus shared by GT911 touch (and any future peripherals).
    i2c_master_bus_config_t bus_cfg = {};
    bus_cfg.clk_source                   = I2C_CLK_SRC_DEFAULT;
    bus_cfg.i2c_port                     = BOARD_TOUCH_I2C_PORT;
    bus_cfg.scl_io_num                   = BOARD_TOUCH_I2C_SCL;
    bus_cfg.sda_io_num                   = BOARD_TOUCH_I2C_SDA;
    bus_cfg.glitch_ignore_cnt            = 7;
    bus_cfg.flags.enable_internal_pullup = true;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &s_i2c_bus));

    // Backlight PWM: 11-bit, 30 kHz, PLL-divided clock (required on P4).
    ledc_timer_config_t timer_cfg = {};
    timer_cfg.speed_mode      = LEDC_LOW_SPEED_MODE;
    timer_cfg.duty_resolution = LEDC_TIMER_11_BIT;
    timer_cfg.timer_num       = LEDC_TIMER_0;
    timer_cfg.freq_hz         = BOARD_BK_PWM_FREQ;
    timer_cfg.clk_cfg         = LEDC_USE_PLL_DIV_CLK;
    ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg));

    ledc_channel_config_t ch_cfg = {};
    ch_cfg.gpio_num   = BOARD_BK_PWM_GPIO;
    ch_cfg.speed_mode = LEDC_LOW_SPEED_MODE;
    ch_cfg.channel    = LEDC_CHANNEL_0;
    ch_cfg.timer_sel  = LEDC_TIMER_0;
    ch_cfg.duty       = 0;
    ch_cfg.hpoint     = 0;
    ESP_ERROR_CHECK(ledc_channel_config(&ch_cfg));

    board_backlight_set(true);
    ESP_LOGI(TAG, "Board init done (ESP32-P4, backlight on)");
}

extern "C" const struct Platform *platform_get(void)
{
    static const struct Platform p = { .is_multitouch = true, .has_usb = true };
    return &p;
}
