# SPDX-License-Identifier: GPL-3.0-or-later
#
# `board` component for the ESP32-2432S028Rv3 ("Cheap Yellow Display",
# CYD2USB) — classic ESP32, ST7789 SPI display + XPT2046 SPI touch, no PSRAM.

idf_component_register(
    SRCS
        "board.cpp"
        "display.cpp"
        "touch.cpp"
    INCLUDE_DIRS "../../../main"   # exports board.h / display.h / touch.h / platform.h
    PRIV_INCLUDE_DIRS "."
    REQUIRES
        driver
        esp_driver_spi
        esp_lcd
        esp_lvgl_port
        lvgl__lvgl
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-error=missing-field-initializers)
