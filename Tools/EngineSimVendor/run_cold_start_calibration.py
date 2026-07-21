#!/usr/bin/env python3
"""Integrity-checked launcher for the deterministic cold-start calibration source."""
from __future__ import annotations

import gzip
import hashlib
from pathlib import Path

_SOURCE_GZIP = Path(__file__).resolve().parent / '_cold_start_sources/run_cold_start_calibration.py.gz'
_SOURCE_GZIP_SHA256 = 'd2608b7de6f844c6ed0651e36d73ad60021838388eff788b26efb92a5e7749bc'
_SOURCE_SHA256 = 'b70387026d05679f5760e1cc1927197c8a14ca91fdac3af1ffbf3aa9a081264b'

_payload = _SOURCE_GZIP.read_bytes()
if hashlib.sha256(_payload).hexdigest() != _SOURCE_GZIP_SHA256:
    raise RuntimeError(f"cold-start source archive hash mismatch: {_SOURCE_GZIP}")
_source = gzip.decompress(_payload)
if hashlib.sha256(_source).hexdigest() != _SOURCE_SHA256:
    raise RuntimeError(f"cold-start source hash mismatch: {_SOURCE_GZIP}")
exec(compile(_source.decode("utf-8"), str(_SOURCE_GZIP), "exec"), globals(), globals())
