
## 13. How touchy-opendeck does it

[`touchy-opendeck`](../rust/touchy-opendeck) maps OpenDeck keys onto
LVGL widgets drawn on the Touchy-Pad LCD. A few choices are worth
calling out because they differ from the simple "USB key matrix" model
above; they all rely on the Stage 71 host-API additions
([host-api.md](host-api.md), [rust-api.md](rust-api.md)).

* **The grid is a *user-screen page*, not a screen file.** Instead of
  uploading a full `Screen` and activating it with `Screen_Load`, the
  plugin uploads only a `LayoutGrid` widget body to
  `F:host/uscr/opendeck.pb` (via `pad.user_screen_save("opendeck",
  &page)`) and then brings it to the front by running an
  `ActionChangeWidgetRef(BY_PATH, "F:host/uscr/opendeck.pb",
  target_id="page")` through the new `RunActionsCmd`
  (`pad.show_user_screen("opendeck")`). This reuses the device's
  default prev/next chrome instead of replacing the whole screen.

* **Usable height ≠ display height.** The default chrome eats the top
  row, and the host only knows the *display* height. The plugin
  reserves a fixed `TOP_ROW_HEIGHT = 32` px and computes
  `rows = (display_height - TOP_ROW_HEIGHT) / KEY_PX` (columns still use
  the full width). This is a stop-gap until the device reports its true
  content area.

* **Device id = the reported serial.** The plugin reads
  `SysBoardInfoResponse.serial` (real hardware: `"t"` + 12 hex MAC
  digits; simulator: `"tsim001"`) and registers the OpenDeck id as
  `tp-<serial>` — stable across re-plug, unlike a bus/address tuple.

* **Unified discovery (USB + sim).** The hot-plug loop calls
  `touchy_pad::discover()`, which returns USB devices *and* a simulator
  entry when `TOUCHY_SIM_URL` is set, so a running `touchy --sim` shows
  up in OpenDeck exactly like hardware. The 1 Hz poll diffs on the
  transport-level `describe()` key, then resolves the true serial id
  during attach.

* **Re-push everything on every (re)connect.** Key images live on the
  `R:` ramdisk (`R:host/opendeck/key_<n>.bin`) to avoid flash wear, but
  that means they vanish on a device reboot. The plugin therefore caches
  the last image bytes OpenDeck sent for each key in a `HashMap<u8,
  Vec<u8>>` and, on every attach, re-writes the `uscr/opendeck.pb` body
  **and** every cached key image. No explicit reload is needed — the
  device auto-redraws a widget when its backing file changes.
