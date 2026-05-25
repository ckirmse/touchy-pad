// SPDX-License-Identifier: GPL-3.0-or-later
//
// Stage 52 — zero-copy ("mmap") image fast path.
//
// When a host-pushed LVGL `.bin` image lives on the RamFs ('R:') drive
// AND its on-disk pixel format already matches the display's native
// color format, we can skip the file→heap copy LVGL's normal decoder
// performs and point `lv_image_set_src` directly at an
// `lv_image_dsc_t` whose `data` field aliases the file's bytes in
// PSRAM. This saves both the malloc/copy and the LVGL decoder pass on
// every screen reload.
//
// Files that live on FlashFs ('F:'), or whose color format differs
// from the display native, fall back to the standard "decode from
// path" route (`lv_image_set_src(img, "F:/host/foo.bin")`). The fast
// path declines silently for those cases (returning false with a
// human-readable `reason_out`) so the caller can log + fall back.

#pragma once

#include "lvgl.h"

#include <cstddef>

// Try to build an `lv_image_dsc_t` that aliases the bytes of
// `wire_path` (a drive-prefixed path like "R:host/images/foo.bin").
//
// On success returns true and fills *dsc_out — its `data` field
// points into the filesystem's storage (do NOT free it) and stays
// valid as long as the file is not overwritten in the FS (Stage 55
// will harden this). `dsc_out` itself is owned by the caller.
//
// On failure returns false and sets *reason_out to a short static
// string explaining why (e.g. "not in mmappable FS",
// "cf 0x14 != display native 0x12"). The string is valid until the
// next try_mmap_image() call on the same thread.
bool try_mmap_image(const char *wire_path,
                    lv_image_dsc_t *dsc_out,
                    const char **reason_out);
