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

// Build every child of `container` (a layout widget — see
// `widget_is_layout`) into `parent`. Picks the right `Layout.children`
// list off `container.which_kind`, then for each child runs the full
// build → apply_styles → apply_placement → optional center sequence.
// Recursively descends into nested layout-widget children. No-op if
// `container` is not a layout widget. Caller must hold the LVGL lock.
void widget_build_children(lv_obj_t *parent, const touchy_Widget &container);

// Build (or rebuild) one LVGL layer from a decoded `touchy_Widget`.
// When the root is a layout widget we configure `parent`'s layout
// manager and instantiate its children directly into `parent`. When it
// is a leaf widget it becomes a child of `parent`. For an empty root
// (which_kind == 0) the call is a no-op.
// `parent` is typically an lv_screen or one of LVGL's persistent layer
// objects. Caller must hold the LVGL lock.
void widget_build_layer(lv_obj_t *parent, const touchy_Widget &root);
