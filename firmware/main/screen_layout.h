// SPDX-License-Identifier: GPL-3.0-or-later
//
// Stage 15 / 20.1 / 24.1 — Layer layout + widget placement.
//
// Two scopes:
//   * `apply_layout()` configures the LVGL layout manager (flex / grid
//     / absolute) on an LVGL container from `touchy.Layer`'s `layout`
//     oneof. Used for the active screen and each populated LVGL
//     persistent layer (top / sys / bottom).
//   * `apply_rect()` / `apply_grid_cell()` place an individual widget
//     within that container's layout, mirroring the `Widget.layout`
//     oneof.

#pragma once

#include "lvgl.h"
#include "widgets.pb.h"
#include "touchy.pb.h"

// Configure the LVGL layout manager attached to `parent` from L.layout
// (flex / grid / absolute). Idempotent — safe to call once per layer
// build.
void apply_layout(lv_obj_t *parent, const touchy_Layer &L);

// Apply `Widget.layout.rect` to `obj`. When `absolute_layout` is true
// (screen has no layout manager) we also write the x/y position; under
// a flex/grid layout the parent owns position so we only set the size.
// Zero/negative width or height becomes LV_SIZE_CONTENT.
void apply_rect(lv_obj_t *obj, const touchy_Widget &w, bool absolute_layout);

// Place `obj` in its parent grid cell using `Widget.layout.cell` (or
// the (0,0) cell with span 1 when no cell is set). All values clamp to
// non-negative; `*_span` defaults to 1.
void apply_grid_cell(lv_obj_t *obj, const touchy_Widget &w);
