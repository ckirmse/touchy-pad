// SPDX-License-Identifier: GPL-3.0-or-later
//
// Stage 24 — ForceRender checkbox widget.
//
// Renders as an LVGL checkbox labelled "Force". While the box is
// checked the firmware schedules a fast LV timer that invalidates the
// active screen every tick, forcing LVGL to redraw continuously at
// the display's maximum rate. Pair with the `Fps` widget for a quick
// "what's the worst-case frame rate of this layout?" benchmark.
//
// Multiple ForceRender checkboxes on one screen all drive the same
// shared "force" state via a refcount: checking any one of them turns
// the global redraw timer on; unchecking the last one turns it off.

#pragma once

#include "lvgl.h"

class ForceRenderWidget {
public:
    explicit ForceRenderWidget(lv_obj_t *parent);

    lv_obj_t *obj() const { return _checkbox; }

private:
    lv_obj_t *_checkbox = nullptr;

    static void _valueChangedCb(lv_event_t *e);
    static void _deleteCb(lv_event_t *e);
};
