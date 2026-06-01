// SPDX-License-Identifier: GPL-3.0-or-later
//
// LVGL display bring-up for the ESP32-2432S028Rv3 ("Cheap Yellow Display").
//
// The 2.8" panel is a standard ST7789 driven over SPI, so unlike the
// JC4827W543's bespoke QSPI NV3041A path this uses the stock esp_lcd SPI
// panel-io + esp_lcd_new_panel_st7789, handed to esp_lvgl_port. There is no
// PSRAM on this module, so the LVGL draw buffers live in internal DMA RAM and
// are deliberately small (a few dozen lines, ping-ponged).

#include "display.h"

#include "board_pins.h"

#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

static const char *TAG = "display";

extern "C" lv_display_t *display_init(void)
{
    // ----- Backlight GPIO high -----
    if (BOARD_LCD_GPIO_BACKLIGHT != GPIO_NUM_NC) {
        gpio_config_t bl = {
            .pin_bit_mask = 1ULL << BOARD_LCD_GPIO_BACKLIGHT,
            .mode         = GPIO_MODE_OUTPUT,
            .pull_up_en   = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type    = GPIO_INTR_DISABLE,
        };
        gpio_config(&bl);
        gpio_set_level(BOARD_LCD_GPIO_BACKLIGHT, 1);
    }

    // ----- SPI bus for the panel -----
    ESP_LOGI(TAG, "Configuring ST7789 SPI panel %dx%d @ %d MHz",
             BOARD_LCD_H_RES, BOARD_LCD_V_RES,
             BOARD_LCD_PIXEL_CLOCK_HZ / 1000000);

    spi_bus_config_t bus_cfg = {};
    bus_cfg.mosi_io_num     = BOARD_LCD_GPIO_MOSI;
    bus_cfg.miso_io_num     = BOARD_LCD_GPIO_MISO;
    bus_cfg.sclk_io_num     = BOARD_LCD_GPIO_SCK;
    bus_cfg.quadwp_io_num   = -1;
    bus_cfg.quadhd_io_num   = -1;
    // Worst-case single transfer is a full-width strip of RGB565 pixels.
    bus_cfg.max_transfer_sz = BOARD_LCD_H_RES * 80 * sizeof(uint16_t);
    ESP_ERROR_CHECK(spi_bus_initialize(BOARD_LCD_SPI_HOST, &bus_cfg,
                                       SPI_DMA_CH_AUTO));

    // ----- Panel IO (command/data over SPI) -----
    esp_lcd_panel_io_handle_t io_handle = nullptr;
    esp_lcd_panel_io_spi_config_t io_cfg = {};
    io_cfg.cs_gpio_num       = BOARD_LCD_GPIO_CS;
    io_cfg.dc_gpio_num       = BOARD_LCD_GPIO_DC;
    io_cfg.spi_mode          = 0;
    io_cfg.pclk_hz           = BOARD_LCD_PIXEL_CLOCK_HZ;
    io_cfg.trans_queue_depth = 10;
    io_cfg.lcd_cmd_bits      = 8;
    io_cfg.lcd_param_bits    = 8;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(
        (esp_lcd_spi_bus_handle_t)BOARD_LCD_SPI_HOST, &io_cfg, &io_handle));

    // ----- ST7789 panel -----
    esp_lcd_panel_handle_t panel = nullptr;
    esp_lcd_panel_dev_config_t panel_cfg = {};
    // GPIO_NUM_NC (-1) tells esp_lcd "no dedicated reset line" — the ST7789 on
    // this module shares the system reset.
    panel_cfg.reset_gpio_num = BOARD_LCD_GPIO_RST;
#if BOARD_LCD_BGR_ORDER
    panel_cfg.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR;
#else
    panel_cfg.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
#endif
    panel_cfg.bits_per_pixel = 16;
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_cfg, &panel));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel, BOARD_LCD_INVERT_COLOR));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel, BOARD_LCD_SWAP_XY));
    ESP_ERROR_CHECK(
        esp_lcd_panel_mirror(panel, BOARD_LCD_MIRROR_X, BOARD_LCD_MIRROR_Y));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel, true));

    // ----- LVGL port task + display -----
    lvgl_port_cfg_t port_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    port_cfg.task_priority   = 4;
    port_cfg.task_stack      = 8 * 1024;
    port_cfg.timer_period_ms = 5;
    ESP_ERROR_CHECK(lvgl_port_init(&port_cfg));

    lvgl_port_display_cfg_t disp_cfg = {};
    disp_cfg.io_handle     = io_handle;
    disp_cfg.panel_handle  = panel;
    // 40 lines per buffer, double-buffered, in internal DMA RAM (no PSRAM).
    disp_cfg.buffer_size   = BOARD_LCD_H_RES * 40;
    disp_cfg.double_buffer = true;
    disp_cfg.hres          = BOARD_LCD_H_RES;
    disp_cfg.vres          = BOARD_LCD_V_RES;
    disp_cfg.monochrome    = false;
    disp_cfg.color_format  = LV_COLOR_FORMAT_RGB565;
    disp_cfg.flags.buff_dma    = true;
    // ST7789 expects big-endian RGB565 on the wire; LVGL renders little-endian.
    disp_cfg.flags.swap_bytes  = true;

    lv_display_t *disp = lvgl_port_add_disp(&disp_cfg);
    if (!disp) {
        ESP_LOGE(TAG, "lvgl_port_add_disp failed");
        return nullptr;
    }
    return disp;
}
