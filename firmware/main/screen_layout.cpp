// SPDX-License-Identifier: GPL-3.0-or-later
//
// Stage 15 / 20.1 — Screen + widget layout. See header.

#include "screen_layout.h"

#include "esp_log.h"
#include "lvgl.h"

static const char *TAG = "screens.layout";

void apply_rect(lv_obj_t *obj, const touchy_Widget &w, bool absolute_layout)
{
    if (w.which_layout != touchy_Widget_rect_tag) return;
    const auto &r = w.layout.rect;
    if (absolute_layout) {
        lv_obj_set_pos(obj, r.x, r.y);
    }
    int32_t w_ = r.w > 0 ? r.w : LV_SIZE_CONTENT;
    int32_t h_ = r.h > 0 ? r.h : LV_SIZE_CONTENT;
    lv_obj_set_size(obj, w_, h_);
}

// Place a widget inside a GRID-layout parent. The grid manager owns size
// and position, so we only forward the cell spec from the protobuf and ask
// LVGL to stretch the widget across its assigned track(s).
//
// Proto3 zero-default handling: in a proto3 binary the wire encoding for a
// scalar field whose value equals its default is omitted entirely. That
// means col=0, row=0, col_span=0, row_span=0 are all indistinguishable from
// "field not present". We therefore treat each value as its proto-comment
// specifies: col/row default to track 0 (top-left), col_span/row_span
// default to 1 (single track). Negative values are also clamped.
void apply_grid_cell(lv_obj_t *obj, const touchy_Widget &w)
{
    int32_t col = 0, row = 0, col_span = 1, row_span = 1;
    if (w.which_layout == touchy_Widget_cell_tag) {
        col      = w.layout.cell.col > 0 ? w.layout.cell.col : 0;
        row      = w.layout.cell.row > 0 ? w.layout.cell.row : 0;
        col_span = w.layout.cell.has_col_span ? w.layout.cell.col_span : 1;
        row_span = w.layout.cell.has_row_span ? w.layout.cell.row_span : 1;
    }
    ESP_LOGI(TAG, "apply_grid_cell id='%s' col=%ld row=%ld col_span=%ld row_span=%ld",
             w.id, (long)col, (long)row, (long)col_span, (long)row_span);
    lv_obj_set_grid_cell(obj,
                         LV_GRID_ALIGN_STRETCH, col, col_span,
                         LV_GRID_ALIGN_STRETCH, row, row_span);
}

namespace {

// Maps LayoutFlex.Flow enum values to lv_flex_flow_t.
lv_flex_flow_t flex_flow_from_proto(touchy_LayoutFlex_Flow f)
{
    switch (f) {
    case touchy_LayoutFlex_Flow_ROW:                 return LV_FLEX_FLOW_ROW;
    case touchy_LayoutFlex_Flow_COLUMN:              return LV_FLEX_FLOW_COLUMN;
    case touchy_LayoutFlex_Flow_ROW_WRAP:            return LV_FLEX_FLOW_ROW_WRAP;
    case touchy_LayoutFlex_Flow_ROW_REVERSE:         return LV_FLEX_FLOW_ROW_REVERSE;
    case touchy_LayoutFlex_Flow_ROW_WRAP_REVERSE:    return LV_FLEX_FLOW_ROW_WRAP_REVERSE;
    case touchy_LayoutFlex_Flow_COLUMN_WRAP:         return LV_FLEX_FLOW_COLUMN_WRAP;
    case touchy_LayoutFlex_Flow_COLUMN_REVERSE:      return LV_FLEX_FLOW_COLUMN_REVERSE;
    case touchy_LayoutFlex_Flow_COLUMN_WRAP_REVERSE: return LV_FLEX_FLOW_COLUMN_WRAP_REVERSE;
    default:                                         return LV_FLEX_FLOW_ROW;
    }
}

}  // namespace

void apply_layout(lv_obj_t *scr, const touchy_Layer &L)
{
    switch (L.which_layout) {
    case touchy_Layer_flex_tag: {
        const touchy_LayoutFlex &fl = L.layout.flex;
        lv_obj_set_flex_flow(scr, flex_flow_from_proto(fl.flow));
        if (fl.gap > 0) {
            lv_obj_set_style_pad_column(scr, fl.gap, 0);
            lv_obj_set_style_pad_row(scr, fl.gap, 0);
        }
        break;
    }
    case touchy_Layer_grid_tag: {
        const touchy_LayoutGrid &g = L.layout.grid;
        // Track templates. `cols` columns split the parent into equal
        // fractional units; `rows` does the same vertically when > 0,
        // otherwise we use a single content-sized row.
        //
        // Proto3 zero-default: `cols=0` is treated as "use 1 column".
        // `rows=0` deliberately means "content-sized single row" per the
        // proto comment; use rows ≥ 1 to get FR-sized rows.
        //
        // The descriptor arrays must outlive the call to
        // lv_obj_set_grid_dsc_array — LVGL only stores the pointer.
        // Since we have one active screen at a time we keep them as
        // static buffers; the next screen_load just rewrites them.
        static int32_t col_dsc[17];
        static int32_t row_dsc[17];
        static int32_t row_dsc_content[2] = { LV_GRID_CONTENT,
                                              LV_GRID_TEMPLATE_LAST };
        int cols = g.cols > 0 ? (int)g.cols : 1;
        if (cols > 16) cols = 16;
        for (int i = 0; i < cols; i++) col_dsc[i] = LV_GRID_FR(1);
        col_dsc[cols] = LV_GRID_TEMPLATE_LAST;

        int32_t *row_ptr;
        int rows = (int)g.rows;
        if (rows > 0) {
            if (rows > 16) rows = 16;
            for (int i = 0; i < rows; i++) row_dsc[i] = LV_GRID_FR(1);
            row_dsc[rows] = LV_GRID_TEMPLATE_LAST;
            row_ptr = row_dsc;
        } else {
            row_ptr = row_dsc_content;
        }
        lv_obj_set_grid_dsc_array(scr, col_dsc, row_ptr);
        lv_obj_set_layout(scr, LV_LAYOUT_GRID);
        if (g.gap > 0) {
            lv_obj_set_style_pad_column(scr, g.gap, 0);
            lv_obj_set_style_pad_row(scr, g.gap, 0);
        }
        break;
    }
    case touchy_Layer_absolute_tag:
    default:
        // No layout manager — widgets place themselves via lv_obj_set_pos.
        break;
    }
}
