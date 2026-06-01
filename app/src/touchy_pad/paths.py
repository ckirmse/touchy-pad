# SPDX-License-Identifier: GPL-3.0-or-later
"""Canonical on-device filesystem paths (Stage 68).

Single source of truth for the drive-prefixed directories the host uses
when talking to the device, shared by ``api/``, ``sim/``, ``client.py``
and ``cli.py`` so the literal ``"F:host/s/"`` etc. never appears inline.

Layout
------
* ``F:host/s/`` — screens. The boot/default chrome lives at
  ``F:host/s/default.pb`` (:data:`DEFAULT_SCREEN_PATH`); it carries the
  persistent prev/next chrome and a ``widget_ref(id="page")`` body that
  pages through :data:`USER_SCREENS_DIR`.
* ``F:host/uscr/`` — "user screens": one widget-layout ``.pb`` per
  top-level page that fills the body of the default chrome screen. Most
  users only ever touch files here.
* ``F:host/w/`` — generic widget-refs (TouchyDeck keys,
  :meth:`Touchy.widget_save`). Unchanged from earlier stages.
* ``F:host/images/`` — image assets.

The on-disk directory moved from ``host/screens/`` → ``host/s/`` in
Stage 68; there is no backward-compatible read of the old path.
"""

from __future__ import annotations

# Drive-prefixed directories (always trailing-slash terminated).
SCREENS_DIR = "F:host/s/"
USER_SCREENS_DIR = "F:host/uscr/"
WIDGETS_DIR = "F:host/w/"
IMAGES_DIR = "F:host/images/"

# Filename of the boot/default chrome screen inside SCREENS_DIR.
DEFAULT_SCREEN_FILE = "default.pb"
DEFAULT_SCREEN_PATH = SCREENS_DIR + DEFAULT_SCREEN_FILE


def screen_path(name: str) -> str:
    """Drive-prefixed path of a screen file, e.g. ``F:host/s/home.pb``."""
    return f"{SCREENS_DIR}{name}.pb"


def user_screen_path(name: str) -> str:
    """Drive-prefixed path of a user-screen page, e.g. ``F:host/uscr/trackpad.pb``."""
    return f"{USER_SCREENS_DIR}{name}.pb"


def widget_path(name: str) -> str:
    """Drive-prefixed path of a generic widget-ref, e.g. ``F:host/w/key0.pb``."""
    return f"{WIDGETS_DIR}{name}.pb"


__all__ = [
    "SCREENS_DIR",
    "USER_SCREENS_DIR",
    "WIDGETS_DIR",
    "IMAGES_DIR",
    "DEFAULT_SCREEN_FILE",
    "DEFAULT_SCREEN_PATH",
    "screen_path",
    "user_screen_path",
    "widget_path",
]
