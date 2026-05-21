"""Tests for the packaged image assets used by Stage 20."""

from __future__ import annotations

import struct

from touchy_pad.images import make_smiley_bmp


def test_make_smiley_bmp_is_well_formed():
    blob = make_smiley_bmp()

    magic, file_size, _, _, pixel_offset = struct.unpack_from("<2sIHHI", blob, 0)
    assert magic == b"BM"
    assert file_size == len(blob)

    (
        info_size,
        width,
        height,
        planes,
        bpp,
        compression,
    ) = struct.unpack_from("<IiiHHI", blob, 14)
    assert info_size == 56  # BITMAPV3INFOHEADER
    assert planes == 1
    assert (width, height) == (16, 16)
    assert bpp == 16
    assert compression == 3  # BI_BITFIELDS
    # 16 px * 2 bytes = 32 bytes/row (already 4-aligned) * 16 rows = 512.
    assert len(blob) - pixel_offset == 512
