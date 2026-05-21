"""Touchy-Pad packaged image assets.

The firmware's LVGL build enables ``CONFIG_LV_USE_BMP`` and renders pixels
in RGB565 (``CONFIG_LV_COLOR_DEPTH_16``). LVGL's BMP decoder is zero-copy
and only works when the file's pixel format matches the framebuffer, so
every BMP uploaded to ``/from_host/`` **must** be 16 bpp with the
``BI_BITFIELDS`` (``0xF800 / 0x07E0 / 0x001F``) channel masks.

Today the host package ships pre-built BMPs as binary resources; an
encoder / PNG-JPEG converter is explicitly deferred to a later stage.
"""

from __future__ import annotations

from importlib import resources

__all__ = ["make_smiley_bmp"]


def make_smiley_bmp() -> bytes:
    """Return the packaged 16×16 yellow smiley as an RGB565 BMP.

    The bytes are read from ``touchy_pad/assets/smiley.bmp``, which is a
    binary resource shipped inside the wheel. Used by ``touchy screens
    demo`` as the contents of ``/from_host/images/smiley.bmp`` on the
    device (Stage 20).
    """
    return resources.files("touchy_pad.assets").joinpath("smiley.bmp").read_bytes()
