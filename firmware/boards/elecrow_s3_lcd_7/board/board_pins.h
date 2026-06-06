// SPDX-License-Identifier: GPL-3.0-or-later
//
// Internal pin map and runtime config for the Elecrow ESP32-S3 7" family.
// Covers v2/v3 ("regular") and "Advance" (v1.0, v1.2) variants, which are
// distinguished at boot time by probing for the Advance-only RTC at 0x51.
//
// This header is private to the board component — main/ never sees it.

#pragma once

#include "driver/gpio.h"
#include "hal/i2c_types.h"

// ---------- Shared across all variants ----------

// Panel resolution — identical on v2, v3, and Advance.
#define BOARD_LCD_H_RES          800
#define BOARD_LCD_V_RES          480
#define BOARD_LCD_PIXEL_CLOCK_HZ (18 * 1000 * 1000)

// PCA9557 IO expander (I2C address and register offsets). Present on v3 regular and Advance v1.0.
#define BOARD_PCA9557_ADDR        0x18
#define BOARD_PCA9557_REG_OUTPUT  1
#define BOARD_PCA9557_REG_CONFIG  3

// STC8H1K28 backlight controller (I2C address). Present on Advance v1.2 only.
// Protocol: 0x10 = on/max, 0x05 = off; to dim, send 0x10 then the level byte.
#define BOARD_STC8H1K28_ADDR      0x30

// GT911 capacitive touch: try primary address first, fall back to secondary.
#define BOARD_TOUCH_I2C_ADDR_PRIMARY  0x5D
#define BOARD_TOUCH_I2C_ADDR_BACKUP   0x14

// The Advance variant has an RTC at 0x51 on a dedicated I2C bus (GPIO15/16).
// We probe this at startup to choose the correct pin/timing config.
#define BOARD_ADVANCE_PROBE_SCL  GPIO_NUM_16
#define BOARD_ADVANCE_PROBE_SDA  GPIO_NUM_15

// ---------- Runtime-resolved config ----------
//
// Populated by board_init(); read by display.cpp and touch.cpp via
// board_get_config(). Using a struct instead of #defines because the
// two variants have different pin assignments and timing values.

struct ElecrowBoardConfig {
    bool is_advance;
    bool is_advance_1_2;  // true = STC8H1K28 backlight; false = PCA9557 IO1 backlight

    // I2C bus shared by GT911 touch and PCA9557 IO expander.
    gpio_num_t i2c_scl;
    gpio_num_t i2c_sda;
    uint32_t   i2c_clk_hz;
    i2c_port_t i2c_port;

    // GT911 INT (polled; GPIO_NUM_NC on Advance where it's not wired out).
    gpio_num_t touch_int_gpio;

    // RGB panel sync / clock / data GPIOs.
    gpio_num_t lcd_hsync;
    gpio_num_t lcd_vsync;
    gpio_num_t lcd_pclk;
    gpio_num_t lcd_de;
    gpio_num_t lcd_data[16];

    // RGB panel timing.
    uint32_t hsync_back_porch;
    uint32_t hsync_front_porch;
    uint32_t hsync_pulse_width;
    uint32_t vsync_back_porch;
    uint32_t vsync_front_porch;
    uint32_t vsync_pulse_width;
    int      pclk_active_neg;  // 1 = sample on falling edge, 0 = rising

    // PCA9557 output register: bit to toggle for backlight on/off.
    uint8_t pca9557_backlight_bit;
    // Initial value of the PCA9557 output latch after board_init() completes.
    uint8_t pca9557_output_shadow;
};

// Returns the config populated by board_init(). Valid only after board_init().
const ElecrowBoardConfig *board_get_config(void);
