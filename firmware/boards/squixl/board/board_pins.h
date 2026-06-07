// SPDX-License-Identifier: GPL-3.0-or-later
//
// Pin map and constants for the SQUiXL (Unexpected Maker).
// ESP32-S3, ST7701S 480×480 RGB panel, GT911 multitouch, LCA9555 IO expander.
// This header is private to the board component — main/ never sees it.

#pragma once

#include "driver/gpio.h"
#include "hal/i2c_types.h"

#include "lca9555.h"

// ---------- I2C bus (shared by GT911 touch + LCA9555 IO expander) ----------

#define BOARD_I2C_SCL      GPIO_NUM_2
#define BOARD_I2C_SDA      GPIO_NUM_1
#define BOARD_I2C_CLK_HZ   400000
#define BOARD_I2C_PORT     I2C_NUM_0

// ---------- LCA9555 16-bit IO expander (I2C address 0x20) ----------

#define BOARD_LCA9555_ADDR    0x20

// LCA9555 pin assignments (matching SQUiXL schematic)
#define BOARD_IOEX_BL_EN      0   // backlight power enable (active high)
#define BOARD_IOEX_LCD_RST    1   // LCD reset (active low)
#define BOARD_IOEX_SPI_MOSI   2   // bit-bang SPI MOSI for ST7701S init
#define BOARD_IOEX_SPI_CLK    3   // bit-bang SPI CLK
#define BOARD_IOEX_SPI_CS     4   // bit-bang SPI CS (active low)
#define BOARD_IOEX_TP_RST     5   // GT911 touch reset (active low)

// ---------- GT911 capacitive touch ----------

#define BOARD_TOUCH_I2C_ADDR_PRIMARY  0x5D
#define BOARD_TOUCH_I2C_ADDR_BACKUP   0x14
#define BOARD_TOUCH_INT_GPIO          GPIO_NUM_3

// ---------- LCD panel (ST7701S, RGB parallel, 480×480) ----------

#define BOARD_LCD_H_RES           480
#define BOARD_LCD_V_RES           480
#define BOARD_LCD_PIXEL_CLOCK_HZ  (12 * 1000 * 1000)

#define BOARD_LCD_HSYNC  GPIO_NUM_48
#define BOARD_LCD_VSYNC  GPIO_NUM_47
#define BOARD_LCD_PCLK   GPIO_NUM_39
#define BOARD_LCD_DE     GPIO_NUM_38

// RGB565 data bus: D0-D4 = B0-B4, D5-D10 = G0-G5, D11-D15 = R0-R4
#define BOARD_LCD_D0   GPIO_NUM_21   // B0
#define BOARD_LCD_D1   GPIO_NUM_18   // B1
#define BOARD_LCD_D2   GPIO_NUM_17   // B2
#define BOARD_LCD_D3   GPIO_NUM_16   // B3
#define BOARD_LCD_D4   GPIO_NUM_15   // B4
#define BOARD_LCD_D5   GPIO_NUM_14   // G0
#define BOARD_LCD_D6   GPIO_NUM_13   // G1
#define BOARD_LCD_D7   GPIO_NUM_12   // G2
#define BOARD_LCD_D8   GPIO_NUM_11   // G3
#define BOARD_LCD_D9   GPIO_NUM_10   // G4
#define BOARD_LCD_D10  GPIO_NUM_9    // G5
#define BOARD_LCD_D11  GPIO_NUM_8    // R0
#define BOARD_LCD_D12  GPIO_NUM_7    // R1
#define BOARD_LCD_D13  GPIO_NUM_6    // R2
#define BOARD_LCD_D14  GPIO_NUM_5    // R3
#define BOARD_LCD_D15  GPIO_NUM_4    // R4

// RGB panel timing values (from Unexpected Maker SQUiXL reference)
#define BOARD_LCD_HSYNC_BACK_PORCH   10
#define BOARD_LCD_HSYNC_FRONT_PORCH  50
#define BOARD_LCD_HSYNC_PULSE_WIDTH   8
#define BOARD_LCD_VSYNC_BACK_PORCH    8
#define BOARD_LCD_VSYNC_FRONT_PORCH   8
#define BOARD_LCD_VSYNC_PULSE_WIDTH   3

// ---------- Backlight PWM (LEDC on GPIO40) ----------

#define BOARD_BL_GPIO  GPIO_NUM_40

// Component-private accessor — used by display.cpp to enable backlight power
// after the ST7701S init sequence completes.
Lca9555 *board_get_ioex(void);
