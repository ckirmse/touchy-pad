// SPDX-License-Identifier: GPL-3.0-or-later
//
// Stage 15 / 20.1 — Screen layout + widget placement.
//
// Two scopes:
//   * `apply_layout()` configures the screen-level LVGL layout manager
//     (flex / grid / absolute) from `touchy.Screen`'s `layout` oneof.
//   * `apply_rect()` / `apply_grid_cell()` place an individual widget
//     within that screen's layout, mirroring the `Widget.layout` oneof.

#pragma once

#include "lvgl.h"
#include "widgets.pb.h"
#include "touchy.pb.h"

// Configure the LVGL layout manager attached to `scr` from S.layout
// (flex / grid / absolute). Idempotent — safe to call once per screen
// build.
void apply_layout(lv_obj_t *scr, const touchy_Screen &S);

// Apply `Widget.layout.rect` to `obj`. When `absolute_layout` is true
// (screen has no layout manager) we also write the x/y position; under
// a flex/grid layout the parent owns position so we only set the size.
// Zero/negative width or height becomes LV_SIZE_CONTENT.
void apply_rect(lv_obj_t *obj, const touchy_Widget &w, bool absolute_layout);

// Place `obj` in its parent grid cell using `Widget.layout.cell` (or
// the (0,0) cell with span 1 when no cell is set). All values clamp to
// non-negative; `*_span` defaults to 1.
void apply_grid_cell(lv_obj_t *obj, const touchy_Widget &w);
