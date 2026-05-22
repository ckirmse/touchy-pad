"""Pseudo-filesystem for the device simulator.

Mirrors the firmware's host-uploaded file area (``/from_host/`` on
device) inside a per-serial subdirectory of the user's cache dir, so
the sim's state survives restarts and isolates between simultaneously-
running sim instances.

Cache root resolution uses :mod:`platformdirs` when available, with a
``$XDG_CACHE_HOME``/``~/.cache`` fallback so the sim still works in
minimal environments (e.g. CI without optional deps installed).
"""

from __future__ import annotations

import os
from pathlib import Path


def default_cache_root() -> Path:
    """Return the user-cache root for sim state.

    Linux:   ``$XDG_CACHE_HOME/touchy-pad/sim`` or ``~/.cache/touchy-pad/sim``
    macOS:   ``~/Library/Caches/touchy-pad/sim``
    Windows: ``%LOCALAPPDATA%\\touchy-pad\\sim``
    """
    try:
        from platformdirs import user_cache_dir
    except ImportError:
        env = os.environ.get("XDG_CACHE_HOME")
        base = Path(env) if env else Path.home() / ".cache"
        return base / "touchy-pad" / "sim"
    return Path(user_cache_dir("touchy-pad", appauthor=False)) / "sim"


class SimFs:
    """A flat key/value store under ``<root>/<serial>/``.

    Keys are virtual paths the host uses on the wire (e.g.
    ``"screens/home.pb"``, ``"images/smiley.png"``). They're written
    verbatim to ``<root>/<serial>/<key>`` with parent directories
    created on demand. Path traversal (``..``) is rejected.
    """

    def __init__(self, root: Path, serial: str) -> None:
        self.root = (root / serial).resolve()
        self.root.mkdir(parents=True, exist_ok=True)

    # -- helpers ----------------------------------------------------------

    def _resolve(self, path: str) -> Path:
        """Map a virtual path to an absolute path inside :attr:`root`.

        Raises ``ValueError`` on traversal attempts so a malicious
        upload can't write outside the sim's sandbox.
        """
        if not path or path.startswith("/") or ".." in Path(path).parts:
            raise ValueError(f"bad sim-fs path: {path!r}")
        abs_path = (self.root / path).resolve()
        # Belt-and-suspenders: even if Path.parts looked clean, confirm
        # the resolved target really sits under root.
        try:
            abs_path.relative_to(self.root)
        except ValueError as exc:
            raise ValueError(f"path escapes sim-fs root: {path!r}") from exc
        return abs_path

    # -- API --------------------------------------------------------------

    def reset(self) -> None:
        """Delete every uploaded file (FileResetCmd analog)."""
        for child in list(self.root.rglob("*")):
            if child.is_file():
                child.unlink()
        for child in sorted(
            (p for p in self.root.rglob("*") if p.is_dir()),
            key=lambda p: len(p.parts),
            reverse=True,
        ):
            try:
                child.rmdir()
            except OSError:
                pass

    def save(self, path: str, data: bytes) -> None:
        """Write *data* to virtual *path*, creating parents."""
        target = self._resolve(path)
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)

    def read(self, path: str) -> bytes:
        return self._resolve(path).read_bytes()

    def exists(self, path: str) -> bool:
        try:
            return self._resolve(path).is_file()
        except ValueError:
            return False

    def list_screens(self) -> list[str]:
        """Return uploaded screen stems, sorted lexicographically.

        Mirrors the firmware's boot-time scan of ``screens/*.pb``; the
        first entry is the default-load target when no last-screen pref
        is recorded.
        """
        d = self.root / "screens"
        if not d.is_dir():
            return []
        return sorted(p.stem for p in d.glob("*.pb"))
