// SPDX-License-Identifier: GPL-3.0-or-later
//
// Stage 20.1 / 24 — per-kind widget construction.
//
// `widget_build()` is the single entry point used by the screen loader:
// it dispatches on `Widget.kind` and returns a freshly-created LVGL
// object (or nullptr for unknown/unsupported kinds). The dispatched
// builders do _not_ apply styles or layout — those are handled by the
// caller in screens.cpp after build returns, so the same code path can
// style every widget uniformly.

#pragma once

#include "lvgl.h"
#include "widgets.pb.h"

// Dispatch on `w.which_kind` and return a new LVGL object parented at
// `parent`, or nullptr if the kind is unknown / cannot be built (in
// which case the caller should skip the widget — apply_styles &
// apply_*layout are also skipped automatically when the return is null).
lv_obj_t *widget_build(lv_obj_t *parent, const touchy_Widget &w);
