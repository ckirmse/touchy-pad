#!/usr/bin/env python3
"""Rewrite flat protoc cross-file imports to package-relative ones.

protoc emits ``import <mod>_pb2 as <mod>_pb2`` for cross-file references.
Rewrite those to ``from . import <mod>_pb2 as <mod>_pb2`` so the _proto/
package works without adding its directory to sys.path.

Usage::

    python fix_proto_imports.py <file1.py> [<file2.py> ...]
"""
import re
import sys

_MODULES = ("touchy_pb2", "widgets_pb2", "preferences_pb2")
_PATTERN = re.compile(r"^import (" + "|".join(_MODULES) + r") as ", re.MULTILINE)


def _fix(path: str) -> None:
    text = open(path).read()
    text = _PATTERN.sub(lambda m: "from . import " + m.group(1) + " as ", text)
    open(path, "w").write(text)


if __name__ == "__main__":
    for p in sys.argv[1:]:
        _fix(p)
