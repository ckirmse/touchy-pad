# touchy-pad — AI Agent Guide

Open-source multitouch USB touchpad / button matrix with a built-in
customisable LCD (ESP32-S3, jc4827w543 or waveshare_s3_lcd_7b boards).
The host-side companion is a Python package (`touchy-pad`) that ships a
CLI (`touchy`), a high-level API, a Tkinter/PySide6 device simulator, and
a StreamDeck-compatibility shim (`TouchyDeck`).

`CLAUDE.md` is a symlink to this file — keep them in sync via the symlink.

## Repo layout
| Path | Purpose |
|------|---------|
| `firmware/main/` | ESP-IDF C++ firmware (CMake, **not** PlatformIO) |
| `firmware/main/main.cpp` | Entry point — keep thin; subsystems live in their own `.cpp/.h` |
| `firmware/boards/<board>/` | Per-board pinout / display / touch drivers |
| `proto/` | Shared protobuf schemas (`touchy.proto`, `widgets.proto`, `preferences.proto`) + nanopb `.options` files |
| `app/src/touchy_pad/` | Python package — `cli.py`, `client.py`, `transport.py`, `api/`, `sim/`, `touchydeck/`, `_proto/` |
| `app/tests/` | pytest suite (host-side only; firmware has no unit tests) |
| `tools/StreamController/` | git submodule, branch `pr-touchypad`, with `touchy_bootstrap.py` shim |
| `tools/streamdeck-probe/` | Stage 50.1 reverse-engineering tool |
| `docs/design.md` | **Authoritative stage history — read before starting new work** |
| `docs/host-api.md` | USB endpoint + protocol spec |
| `Justfile` | All build/test/run tasks — prefer `just <recipe>` over raw commands |
| `VERSION` | Single-source version (read by Python + CMake) |

## Implementation status
All stages 0–24.4, 50.2, 51, 64.1, 64.3, 64.4, 65, 65.1, and 67 are **done**. Latest active wire-format:
`Screen.Version.CURRENT == 5`, `SysBoardInfoResponse.ProtocolVersion.CURRENT == 6`.
Highlights worth remembering:

- **Boards span two chips (Stage 65).** ESP32-S3 boards (`jc4827w543`,
  `waveshare_s3_lcd_7b`) have native USB; the classic-ESP32
  `esp32_2432s028rv3` ("CYD2USB") does not. Each board declares its chip
  in a one-line `firmware/boards/<board>/target` file; USB capability is
  keyed off `CONFIG_SOC_USB_OTG_SUPPORTED` (not a custom flag). `just
  firmware-reconfigure [board]` reads `target` and runs `idf.py
  -DBOARD=<board> set-target <chip>` — the `-DBOARD` is required, and
  `rm -f firmware/sdkconfig firmware/sdkconfig.<board>` before switching
  chips. `firmware/main/platform.{h,cpp}` exposes `platform_get()` →
  `{is_multitouch, has_usb}`, surfaced over proto as
  `SysBoardInfoResponse.is_multitouch` / `has_usb`.

- **The "CYD" family shares one C++ implementation (Stage 65.1).** All
  classic-ESP32 CYD boards (`esp32_2432s028rv3` 2.8" ST7789,
  `esp32_2432s024` 2.4" ILI9341, more coming) compile the *same* sources in
  `firmware/boards/cyd_common/` (`board.cpp`, `display.cpp`, `touch.cpp`).
  Each board dir keeps only its `board_pins.h` + a tiny
  `board/CMakeLists.txt` (which lists `../../cyd_common/*.cpp` and sets
  `PRIV_INCLUDE_DIRS "."` so the board's own `board_pins.h` wins) +
  `idf_component.yml`. No symlinks. The panel driver is chosen at compile
  time from `board_pins.h`: `cyd_common/display.cpp` keys off
  `BOARD_LCD_CONTROLLER_ILI9341` (pulls the `espressif/esp_lcd_ili9341`
  managed component, `esp_lcd_new_panel_ili9341`) vs the default
  `BOARD_LCD_CONTROLLER_ST7789` (in-tree `esp_lcd_new_panel_st7789`); bring-up
  is otherwise identical. ILI9341 typically wants `BOARD_LCD_INVERT_COLOR=0`
  where ST7789 wants `1`.

- USB device is a composite class: CDC-ACM + HID (mouse + keyboard via
  report IDs 1/2) + vendor-class bulk pair (command/response) + interrupt-IN
  mailbox endpoint (0x85) that just signals "events available".
- Host ↔ device wire protocol = self-synchronising frames
  `MAGIC(0xA5 0x5A) | LEN(u16 LE) | payload | CRC8` (Stage 64.3) over the
  bulk pair. Identical framing on every transport (USB, simulator TCP,
  serial). One decoder per side: `firmware/main/host_api.cpp` (the
  `HostApiLink` abstraction), `app/src/touchy_pad/transport.py`
  (`_StreamFramedTransport` / `_FrameDecoder`), and
  `rust/touchy-pad/src/transport.rs` (`FrameDecoder`). The serial
  transport (`transport_serial.py`; Rust `transport_serial.rs` behind the
  `serial` feature) always runs at 115200 baud and carries only protocol
  frames — device logs ride the Stage 64.1 `LogRecord` tunnel, never raw
  text on the protocol port. Firmware serial path is gated on
  `CONFIG_TOUCHY_PROTO_OVER_SERIAL` (default n; `y` for the CYD board,
  which has a `UartLink` on `UART_NUM_0` gated `#if
  CONFIG_TOUCHY_PROTO_OVER_SERIAL && !CONFIG_SOC_USB_OTG_SUPPORTED`).
  `firmware/main/CMakeLists.txt` REQUIRES `esp_driver_uart` for this
  (IDF v6 split out `driver/uart.h`). Gotcha: in `sdkconfig.defaults`,
  `# CONFIG_X is not set` is **not** a comment — it's the `X=n` directive
  and will silently override an earlier `CONFIG_X=y`.
- nanopb uses `FT_POINTER` (heap) for `repeated` widget/action/step
  fields and the `FileWrite` payload. RAII via `PbMessage<T>` in
  `firmware/main/protobuf.h`.
- Filesystem paths are drive-prefixed: `F:host/...` = LittleFS (persistent
  flash), `R:host/...` = ramdisk (transient, e.g. image assets). RamFs
  prefers `MALLOC_CAP_SPIRAM` but falls back to internal RAM, so the `R:`
  drive and image assets work even on no-PSRAM boards (CYD, ~520 KB SRAM).
- Stage 21 (Python CLI for layouts) is implemented as `touchy screens push`,
  consuming the `touchy_pad.api.screens` DSL (`button`, `slider`, `toggle`,
  `image_button`, `trackpad`, `log_line`, layout helpers `row`/`col`/`grid`).
- Stage 67 lets `host_action(on_event=lambda e: ...)` attach a callback
  inline. Codes auto-allocate from `_events.AUTO_CODE_BASE` (`0x10000`);
  manual codes stay below that. The callable is stashed in
  `app/src/touchy_pad/api/_events.py` keyed by code, then
  `Touchy.screen_save`/`widget_save` walk the proto tree
  (`_collect_host_codes`), `harvest()` the matching bindings, and register
  them via `on_host_event` — so inline callbacks light up on upload.
- Stage 30 simulator lives in `app/src/touchy_pad/sim/` (Tkinter/PySide6).
  Invoke with `touchy --sim ...` (or `--sim-headless` for CI).
- Stage 50.2 StreamDeck shim is `touchy_pad.touchydeck.TouchyDeck`;
  `touchy_pad.touchydeck.install()` monkey-patches
  `StreamDeck.DeviceManager.enumerate`. **Must be called explicitly** —
  no import side-effects. See `tools/StreamController/touchy_bootstrap.py`.
- Stage 64.1 tunnels device ESP_LOG output back to the host over the
  same `EventConsumeCmd` poll: when the event queue is empty but a log
  is waiting the Response's `payload` oneof carries `LogRecord` (tag 5)
  instead of `EventConsumeResponse`. Firmware hook is
  `firmware/main/log_proto.{h,cpp}` (gated on `CONFIG_TOUCHY_LOG_OVER_PROTO`,
  default `y` in `firmware/sdkconfig.defaults`). Host dispatchers:
  `TouchyClient._dispatch_log_record` → `touchy_pad.device` logger;
  Rust `dispatch_log_record` → `log` crate at target `touchy_pad::device::<TAG>`.

## Build & test
Everything goes through Just; never run raw `idf.py` / `poetry` /
`protoc` unless a recipe is clearly missing:

```bash
just init              # one-time devcontainer setup
just build-proto       # regenerate Python + C nanopb bindings
just app-test          # pytest (proto bindings auto-rebuilt)
just app-lint          # ruff format + lint
just app-run -- ...    # invoke the `touchy` CLI inside Poetry
just firmware-build    # ESP-IDF build for the current board
just flash             # build + flash
just streamcontroller-run [--sim | --sim-headless]
```

CI: `.github/workflows/app-ci.yml` runs `build-app` on
ubuntu/windows/macos. **Windows has no libusb** — any code path that
touches `usb.core.find()` must guard against `NoBackendError`
(not just `ImportError`). See `app/src/touchy_pad/api/device.py` and
`app/src/touchy_pad/touchydeck/discovery.py` for the pattern.

## Justfile gotchas (learned the hard way)
- All recipe bodies use `#!/usr/bin/env bash` shebangs and must use
  **relative paths** — `justfile_directory()` produces `D:\a\...` on
  Windows, which bash interprets `\a` as `a`.
- Use `${SYS_PYTHON:-/usr/bin/python3}` inside recipes (read at runtime),
  not `{{sys_python}}` (expanded by Just at parse time).
- macOS BSD `paste` needs the explicit `-` stdin marker:
  `... | paste -sd: -`.

## Coding conventions
- **Device:** C++ via ESP-IDF (no Arduino), LVGL primitives only (no
  direct framebuffer writes). New subsystems → own `.cpp/.h` pair in
  `firmware/main/`. Long-running work → its own FreeRTOS task.
- **Host:** Python 3.11+, Poetry, ruff (format + lint), pytest. Public
  API lives under `touchy_pad.api`; the high-level entry is
  `touchy_pad.api.touchy_open()`.
- **Logging:** use `logging.getLogger(__name__)`. High-frequency RPC
  trace lines go on a child logger (e.g. `touchy_pad.client.rpc`) so
  callers can silence them independently. Python stdlib has no TRACE
  level — prefer child loggers over custom levels.
- **NotImplementedError in subclass-required methods:** prefer logging
  ERROR + returning a sensible default over raising, so optional
  StreamDeck features don't crash StreamController introspection.

## Hardware
- Display + touch panel ride a shared I²C-ish interface (board-specific);
  see `firmware/boards/<board>/`. GT911 multitouch on jc4827w543 /
  waveshare. The CYD boards (`esp32_2432s028rv3` 2.8" ST7789,
  `esp32_2432s024` 2.4" ILI9341) are an SPI panel over SPI2 +
  XPT2046 resistive single-touch over SPI3 (managed component
  `atanisoft/esp_lcd_touch_xpt2046`); BGR/INVERT/SWAP/MIRROR + backlight
  GPIO live in each `board/board_pins.h`, and the panel controller is
  selected there (`BOARD_LCD_CONTROLLER_ST7789` / `_ILI9341`). Note
  `reset_gpio_num` is `gpio_num_t` in IDF v6 — assign the enum directly.
- Optional haptics: DRV2605L on a separate I²C bus (not yet wired).
- USB-OTG controller exposes one IN/OUT bulk pair + one interrupt-IN
  endpoint for the vendor interface (no second IN for events — hence the
  mailbox-poll design).

## Workflow rules
- **Never auto-commit or push.** Make changes; let the user commit.
- `docs/design.md` is the source of truth for "what stage are we on" —
  update it when you finish a stage.
- The git submodule at `tools/StreamController` tracks branch
  `pr-touchypad`; `just streamcontroller-run` does
  `git submodule update --init --remote` first.
