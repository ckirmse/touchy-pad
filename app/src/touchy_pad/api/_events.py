"""Auto-allocated host-event codes and their pending callbacks (Stage 67).

A :func:`touchy_pad.api.screens.host_action` may be given an inline
``on_event=`` callback instead of (or alongside) an explicit numeric
``code``. Because a ``_proto.Action`` is pure protobuf it cannot carry a
Python callable, so the callback is stashed here keyed by an
auto-allocated code, and the runtime :class:`touchy_pad.api.device.Touchy`
harvests the matching bindings when the screen / widget that references
them is uploaded.

This module is intentionally tiny and import-free (beyond the stdlib) so
both ``screens.py`` (which populates the registry) and ``device.py``
(which drains it) can import it without a cycle.
"""

from __future__ import annotations

import itertools
import threading
from collections.abc import Callable
from typing import TYPE_CHECKING

if TYPE_CHECKING:  # pragma: no cover — typing only.
    from .. import _proto

HostEventCallback = Callable[["_proto.LvEvent"], None]

#: Auto-allocated host codes start here. Manual codes passed to
#: :func:`host_action` should stay **below** this value so the two ranges
#: never collide. The base is deliberately small (``0x10000``) so the
#: varint-encoded ``ActionHost.code`` stays compact on the wire.
AUTO_CODE_BASE: int = 0x10000

_lock = threading.Lock()
_counter = itertools.count(AUTO_CODE_BASE)
# Pending auto-bindings awaiting harvest by an upload. Maps code -> callback.
_pending: dict[int, HostEventCallback] = {}


def alloc_code(callback: HostEventCallback | None = None) -> int:
    """Allocate a fresh, unique host code in the auto range.

    If *callback* is given it is registered as the pending binding for the
    new code, to be picked up by the next upload that references it.
    """
    with _lock:
        code = next(_counter)
        if callback is not None:
            _pending[code] = callback
    return code


def register_binding(code: int, callback: HostEventCallback) -> None:
    """Record *callback* as the pending binding for an explicit *code*."""
    with _lock:
        _pending[code] = callback


def harvest(codes: set[int]) -> dict[int, HostEventCallback]:
    """Pop and return every pending binding whose code is in *codes*.

    Called by an upload once it knows which host codes a screen / widget
    actually references, so callbacks light up exactly when (and on
    whichever device) the screen is sent.
    """
    out: dict[int, HostEventCallback] = {}
    with _lock:
        for code in codes:
            cb = _pending.pop(code, None)
            if cb is not None:
                out[code] = cb
    return out


def _reset() -> None:
    """Test helper: clear the counter and all pending bindings."""
    global _counter
    with _lock:
        _counter = itertools.count(AUTO_CODE_BASE)
        _pending.clear()
