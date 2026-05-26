# Display performance notes (JC4827W543 / NV3041A)

Captured from a tuning investigation in May 2026. Useful background for
anyone wanting to push the frame-rate higher on the CYD board in the
future.

## Current measured baseline

- **~24 FPS** on the "force fullscreen redraw" demo (worst case — every
  pixel changes every frame, no help from LVGL dirty-region tracking).
- Real-world UI redraws are much faster because LVGL only flushes the
  invalidated rectangle.

## Theoretical ceiling

- Panel: NV3041A 480×272 RGB565, QSPI 4 lines, MIPI-like protocol.
- Pixel clock: `BOARD_LCD_QSPI_CLK_HZ = 32 MHz` in
  [firmware/boards/jc4827w543/board/board_pins.h](../firmware/boards/jc4827w543/board/board_pins.h).
- The NV3041A datasheet quotes ~40 MHz QSPI, but on this board anything
  above 32 MHz produces visible corruption / dropped pixels. **Do not
  raise this without re-validating on real hardware.** See
  [hardware.md](hardware.md#nv3041a-qspi-clock-ceiling).
- Wire bandwidth at 32 MHz × 4 lines = **16 MB/s**.
- One full-screen RGB565 frame = 480 × 272 × 2 = 261 120 bytes ≈ 255 KB.
- Hard wire-rate ceiling: ~16 MB/s ÷ 255 KB ≈ **~61 FPS**.

The 24 FPS we measure is well under the wire ceiling, meaning the
bottleneck for the worst-case demo is **on the CPU side**, not on the
SPI bus.

## What's already implemented (don't redo these)

Audited in [firmware/boards/jc4827w543/board/display.cpp](../firmware/boards/jc4827w543/board/display.cpp)
and [nv3041a.c](../firmware/boards/jc4827w543/board/nv3041a.c):

1. **DMA-capable draw buffers.** Both LVGL partial buffers are allocated
   with `heap_caps_malloc(..., MALLOC_CAP_DMA)`, so they land in
   internal SRAM. PSRAM on this board (`CONFIG_SPIRAM_MODE_OCT=y`) is
   **not** DMA-capable — any small register payload that's stack-resident
   must use `SPI_TRANS_USE_TXDATA`. See the `nv_write_reg_bytes_inline`
   helper in `nv3041a.c` for why this matters (silently broken CASET /
   RASET on flush #2+ when payloads land in PSRAM).
2. **Double-buffered ping-pong.** Two equal partial buffers passed to
   `lv_display_set_buffers(..., LV_DISPLAY_RENDER_MODE_PARTIAL)`. LVGL
   renders into buffer B while the SPI peripheral DMAs buffer A.
3. **ISR-driven flush-ready.** `nv_post_cb` runs from the SPI ISR after
   the last chunk hits the wire and calls `lv_display_flush_ready()`
   directly — no LVGL-task polling.
4. **Partial-region flush.** The flush callback passes
   `area->x1..y2` straight to `nv3041a_set_window`, so only the dirty
   rectangle goes over the wire.
5. **Bus held for device lifetime** + 32-slot transaction pool +
   `SPI_TRANS_CS_KEEP_ACTIVE` across chunks — required to keep CS
   asserted across a multi-chunk memory write.

## Where the remaining CPU cost actually goes

For a *full-screen* redraw, on the LVGL task, in order:

1. LVGL software rendering into the draw buffer (`LV_DRAW_SW_COMPLEX=y`).
2. **`lv_draw_sw_rgb565_swap(px_map, n)`** — byte-swap each pixel before
   queueing the SPI transactions. ~1–2 ms per full frame on this S3.
3. `nv3041a_set_window` (two tiny register writes).
4. `nv3041a_write_pixels` — chunked into 16 KB QSPI bursts.

The swap exists because LVGL v9 dropped the `LV_COLOR_16_SWAP` Kconfig.
The NV3041A wants big-endian RGB565 on the wire; LVGL renders
little-endian; the swap reconciles them.

## Tuning levers (ranked by effort vs. payoff)

### Cheap, low-risk

1. **Enable `CONFIG_LV_USE_PERF_MONITOR=y`** in
   [firmware/sdkconfig.defaults](../firmware/sdkconfig.defaults). LVGL
   draws an on-screen FPS / CPU% overlay. This is the single best
   diagnostic for figuring out whether you're SPI-bound, swap-bound, or
   render-bound. Currently `=n`.
2. **Bump `LINES_PER_BUF`** in `display.cpp` from 40 → 80 or 120. Halves
   the number of `CASET`/`RASET`/`RAMWR` register-write overhead
   transactions per full-screen redraw, lets LVGL render bigger
   contiguous tiles. Costs internal SRAM:
   - 80 lines = 76 KB total (× 2 buffers)
   - 120 lines = 115 KB total

   Both fit. Expect a few percent win, no risk.

### Medium effort

3. **Eliminate the RGB565 byte-swap.** This is the only real CPU work
   left in the per-flush hot path, and it serialises render → swap →
   enqueue (partly defeating the ping-pong). Options:
   - **No clean LVGL-v9 config switch** — the old `LV_COLOR_16_SWAP`
     Kconfig was removed.
   - **No ESP32-S3 hardware byte-swap** is available for
     arbitrary-length QSPI data. `SPI_DEVICE_TXBIT_LSBFIRST` is bit
     order, not byte order. The GDMA peripheral doesn't byte-swap
     either.
   - **Render LVGL into a "swapped" buffer via a custom render-buffer
     hook** — would need to dig into LVGL v9 internals. Not currently
     supported by a stable public API.

   In other words: stuck with this until LVGL exposes a hook or we move
   to a parallel-RGB / i80 panel that doesn't need the swap.

### Big architectural moves

4. **Raise the QSPI clock.** Theoretical home run (32 → 40 MHz = 25 %
   more wire bandwidth), but **empirically broken on this board** — see
   [hardware.md](hardware.md#nv3041a-qspi-clock-ceiling). Other CYD
   variants with the same panel may tolerate 40 MHz; this one doesn't.
5. **Switch to a parallel RGB / i80 panel** — uses the ESP32-S3's
   `LCD_CAM` peripheral, which has its own dedicated DMA path and can
   push far more pixels/sec than any QSPI panel. Different board
   though.

## Alternative driver: refactor on top of `esp_lcd`

We considered rewriting [nv3041a.c](../firmware/boards/jc4827w543/board/nv3041a.c)
on top of ESP-IDF's [`esp_lcd`](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/lcd/index.html)
API. The relevant entry point is:

```c
esp_lcd_new_panel_io_spi(bus, &io_config, &io);   // io_config.flags.quad_mode = 1
```

Producing an `esp_lcd_panel_io_handle_t` with:
- `esp_lcd_panel_io_tx_param(io, cmd, buf, n)` — sync register write.
- `esp_lcd_panel_io_tx_color(io, cmd, pixels, nbytes)` — async DMA pixel
  stream; fires `on_color_trans_done` from ISR.

NV3041A-style QSPI panels use `lcd_cmd_bits = 32`, `lcd_param_bits = 8`,
with register writes encoded as `(0x02<<24)|(reg<<8)` and pixel streams
as `(0x32<<24)|(0x3C<<8)` — exactly what `nv3041a.c` implements by hand.

A community panel driver exists:
[**eric-c-e/esp_lcd_nv3041**](https://components.espressif.com/components/eric-c-e/esp_lcd_nv3041)
(lightly used — 39 downloads at the time of investigation). API shape
is correct but adoption is small, so I'd write our own thin
`esp_lcd_panel_nv3041a.c` wrapper rather than take the dependency.

The ESP-BSP / `esp_lvgl_port` managed component (already vendored under
[firmware/managed_components/](../firmware/managed_components/)) plugs
directly into an `esp_lcd_panel_handle_t` and handles the LVGL
flush_cb + tick + buffer allocation for you.

### Why we did NOT switch

Under the hood, `esp_lcd_panel_io_spi` calls the **same
`spi_device_queue_trans()` path** with the **same GDMA backend** we're
already using. There is no faster path. The wire is still 32 MHz × 4
lines. None of the levers above change.

**Real upsides of switching**, if someone wants to do it later:
- Delete ~300 lines of fiddly DMA / CS / queue plumbing.
- Use the same `esp_lvgl_port` integration that every other ESP-IDF
  LVGL project uses (easier upstream support).
- `esp_lcd_panel_draw_bitmap(panel, x1, y1, x2+1, y2+1, buf)` replaces
  `nv3041a_set_window` + `nv3041a_write_pixels`.

**Real downsides:**
- Pure code-quality refactor, **zero throughput gain**.
- Risk of regressing the PSRAM-vs-DMA gotcha that
  `nv_write_reg_bytes_inline` documents — `esp_lcd`'s internals are
  worth re-auditing for that.
- Need to validate that `esp_lcd`'s init-table mechanism handles the
  NV3041A's `0xff, 0xa5` "enter extended command set" prefix correctly.

If you want to attempt it, suggested approach:

1. Write `firmware/boards/jc4827w543/board/esp_lcd_panel_nv3041a.{c,h}`
   with an `esp_lcd_new_panel_nv3041a(io, panel_dev_config, &out)` that
   runs the existing init table via `esp_lcd_panel_io_tx_param()` and
   implements `draw_bitmap`, `reset`, `init`, `mirror`, `disp_on_off`.
   ~150 lines.
2. Update `display.cpp` to wire SPI bus → `esp_lcd_new_panel_io_spi`
   (quad mode) → `esp_lcd_new_panel_nv3041a` → `esp_lvgl_port` (or
   directly into LVGL's flush_cb via `on_color_trans_done`).
3. Delete `nv3041a.c` / `nv3041a.h`.
4. Keep `BOARD_LCD_QSPI_CLK_HZ` at 32 MHz.

## Quick reference: relevant config

| Symbol | Value | Where |
|---|---|---|
| `BOARD_LCD_QSPI_CLK_HZ` | `32 MHz` | [board_pins.h](../firmware/boards/jc4827w543/board/board_pins.h) |
| `LINES_PER_BUF` | `40` | [display.cpp](../firmware/boards/jc4827w543/board/display.cpp) |
| `NV3041A_POOL_SIZE` | `32` | [nv3041a.c](../firmware/boards/jc4827w543/board/nv3041a.c) |
| `CHUNK_PIXELS` | `8192` (16 KB) | [nv3041a.c](../firmware/boards/jc4827w543/board/nv3041a.c) |
| `CONFIG_LV_COLOR_DEPTH` | `16` | [firmware/sdkconfig.defaults](../firmware/sdkconfig.defaults) |
| `CONFIG_LV_USE_PERF_MONITOR` | `n` (flip to `y` to measure) | [firmware/sdkconfig.defaults](../firmware/sdkconfig.defaults) |
| `CONFIG_LV_DRAW_SW_COMPLEX` | `y` | [firmware/sdkconfig.defaults](../firmware/sdkconfig.defaults) |
| `CONFIG_SPIRAM_MODE_OCT` | `y` | [firmware/boards/jc4827w543/sdkconfig.defaults](../firmware/boards/jc4827w543/sdkconfig.defaults) |
| `CONFIG_SPIRAM_SPEED_80M` | `y` | [firmware/boards/jc4827w543/sdkconfig.defaults](../firmware/boards/jc4827w543/sdkconfig.defaults) |
