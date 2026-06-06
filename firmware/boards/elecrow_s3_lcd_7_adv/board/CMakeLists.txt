# SPDX-License-Identifier: GPL-3.0-or-later
#
# `board` component for Elecrow ESP32-S3 7" Advance (v1.0, v1.2).
# Reuses source files from elecrow_s3_lcd_7/board/ — same runtime auto-detection
# logic, different sdkconfig (USB enabled, serial transport not required).

set(ELECROW_SRC "${CMAKE_CURRENT_SOURCE_DIR}/../../elecrow_s3_lcd_7/board")

idf_component_register(
    SRCS
        "${ELECROW_SRC}/board.cpp"
        "${ELECROW_SRC}/display.cpp"
        "${ELECROW_SRC}/touch.cpp"
    INCLUDE_DIRS "../../../main"          # board.h / display.h / touch.h
    PRIV_INCLUDE_DIRS "${ELECROW_SRC}"   # board_pins.h
    REQUIRES
        driver
        esp_driver_ledc
        esp_lcd
        esp_lvgl_port
        esp_psram
        lvgl__lvgl
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-error=missing-field-initializers)
