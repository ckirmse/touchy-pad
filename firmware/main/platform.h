// SPDX-License-Identifier: GPL-3.0-or-later
//
// Per-board capability descriptor (Stage 65).
//
// Some host-visible behaviour depends on what the *board* can physically do
// rather than on what the firmware was compiled with — e.g. whether the
// touch panel reports more than one simultaneous contact (capacitive GT911
// vs. resistive XPT2046), or whether the chip has a native USB-OTG port at
// all. `fill_board_info()` ships these flags to the host so the companion
// app / simulator can adapt (e.g. refuse multi-finger gestures on a
// single-touch panel) without hard-coding board names.
//
// Each board provides its own `platform_get()` in boards/<board>/board.cpp.

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

struct Platform {
    // True when the touch panel can report >1 simultaneous touch point.
    bool is_multitouch;
    // True when the chip exposes a native USB-OTG port (HID + vendor-bulk
    // host transport). False on classic-ESP32 boards that reach the host
    // only through a UART bridge. Mirrors CONFIG_SOC_USB_OTG_SUPPORTED.
    bool has_usb;
};

// Returns this board's capability descriptor. Implemented per-board; the
// returned pointer is to static storage and never NULL.
const struct Platform *platform_get(void);

#ifdef __cplusplus
}
#endif
