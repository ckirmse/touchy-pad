// SPDX-License-Identifier: GPL-3.0-or-later
//
// XPT2046 resistive-touch bring-up for the ESP32-2432S028Rv3 ("CYD").
//
// The XPT2046 hangs off its own dedicated SPI bus (separate from the ST7789
// display bus), so we initialise that bus here, attach the touch panel-io,
// create the esp_lcd_touch handle, and register it with esp_lvgl_port. Being
// resistive it reports a single contact point — TOUCH_MAX_POINTS still bounds
// the buffer the trackpad widget reads, but only one slot is ever populated.

#include "touch.h"

#include "board_pins.h"

#include "esp_log.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_touch_xpt2046.h"
#include "esp_lvgl_port.h"
#include "driver/spi_master.h"

static const char *TAG = "touch";

static lv_indev_t *s_indev = nullptr;

lv_indev_t *touch_get_indev(void) { return s_indev; }

extern "C" esp_lcd_touch_handle_t touch_init(lv_display_t *disp)
{
    ESP_LOGI(TAG, "Initialising XPT2046 (CS=%d IRQ=%d)",
             (int)BOARD_TOUCH_GPIO_CS, (int)BOARD_TOUCH_GPIO_IRQ);

    // ----- Dedicated SPI bus for the touch controller -----
    spi_bus_config_t bus_cfg = {};
    bus_cfg.mosi_io_num   = BOARD_TOUCH_GPIO_MOSI;
    bus_cfg.miso_io_num   = BOARD_TOUCH_GPIO_MISO;
    bus_cfg.sclk_io_num   = BOARD_TOUCH_GPIO_SCK;
    bus_cfg.quadwp_io_num = -1;
    bus_cfg.quadhd_io_num = -1;
    ESP_ERROR_CHECK(spi_bus_initialize(BOARD_TOUCH_SPI_HOST, &bus_cfg,
                                       SPI_DMA_CH_AUTO));

    // ----- Touch panel IO -----
    esp_lcd_panel_io_handle_t tp_io = nullptr;
    esp_lcd_panel_io_spi_config_t io_cfg = ESP_LCD_TOUCH_IO_SPI_XPT2046_CONFIG(
        BOARD_TOUCH_GPIO_CS);
    io_cfg.pclk_hz = BOARD_TOUCH_PIXEL_CLOCK_HZ;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(
        (esp_lcd_spi_bus_handle_t)BOARD_TOUCH_SPI_HOST, &io_cfg, &tp_io));

    // ----- esp_lcd_touch handle -----
    esp_lcd_touch_config_t tp_cfg = {};
    tp_cfg.x_max        = BOARD_LCD_H_RES;
    tp_cfg.y_max        = BOARD_LCD_V_RES;
    tp_cfg.rst_gpio_num = GPIO_NUM_NC;
    tp_cfg.int_gpio_num = BOARD_TOUCH_GPIO_IRQ;
    // The XPT2046 raw axes are rotated/mirrored relative to the (already
    // rotated) ST7789 framebuffer; match the display orientation so taps land
    // under the finger. Tune alongside the BOARD_LCD_* orientation flags.
    tp_cfg.flags.swap_xy  = BOARD_LCD_SWAP_XY;
    tp_cfg.flags.mirror_x = BOARD_LCD_MIRROR_X;
    tp_cfg.flags.mirror_y = BOARD_LCD_MIRROR_Y;

    esp_lcd_touch_handle_t tp = nullptr;
    ESP_ERROR_CHECK(esp_lcd_touch_new_spi_xpt2046(tp_io, &tp_cfg, &tp));

    lvgl_port_touch_cfg_t lv_cfg = {};
    lv_cfg.disp   = disp;
    lv_cfg.handle = tp;
    s_indev = lvgl_port_add_touch(&lv_cfg);
    if (s_indev == nullptr) {
        ESP_LOGW(TAG, "lvgl_port_add_touch failed");
    }
    return tp;
}
