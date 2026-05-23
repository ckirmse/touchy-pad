// SPDX-License-Identifier: GPL-3.0-or-later
//
// Stage 24 — live frames-per-second readout widget.
//
// A small label that periodically updates with the LVGL display's
// current refresh rate. Useful while iterating on screen layouts to
// see the cost of expensive widgets (image scaling, transitions, ...).
//
// Implementation: an LVGL timer fires twice per second on the LVGL
// task, samples `lv_display_get_refresh_period_ms()` for the screen's
// display, and rewrites the label. Both the timer and the label are
// torn down via an LV_EVENT_DELETE callback on the container so the
// host can swap screens freely.

#pragma once

#include "lvgl.h"

class FpsWidget {
public:
    explicit FpsWidget(lv_obj_t *parent);

    lv_obj_t *obj() const { return _container; }

private:
    // Heap context owned by the widget's LV_EVENT_DELETE handler so the
    // display event hook and the LVGL timer can both be torn down even
    // though _deleteCb is a free static function (it has no `this`).
    struct DeleteCtx {
        uint32_t     *counter;
        lv_timer_t   *timer;
        lv_display_t *disp;
    };

    lv_obj_t   *_container = nullptr;
    lv_obj_t   *_label     = nullptr;
    lv_timer_t *_timer     = nullptr;
    // Pointer into DeleteCtx::counter so _tick() (called via the timer
    // on the LVGL task) can read + reset the frame counter without
    // round-tripping through user_data.
    uint32_t   *_counter   = nullptr;

    void _tick();
    static void _timerCb(lv_timer_t *t);
    static void _deleteCb(lv_event_t *e);
};
