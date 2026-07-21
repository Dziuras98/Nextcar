#!/usr/bin/env python3
"""Integrity-checked launcher for the deterministic cold-start calibration source."""
from __future__ import annotations

import gzip
import hashlib
from pathlib import Path

_SOURCE_GZIP = Path(__file__).resolve().parent / '_cold_start_sources/record_cold_start_calibration_evidence.py.gz'
_SOURCE_GZIP_SHA256 = '1c2d368cdff0e817dcff5e02934a593ad6d53b9b3c109c0905edddef8054c27c'
_SOURCE_SHA256 = 'f1ee52d8d2df40df5dad19aaf97bf796cf4ef32c1bd5b60700fec4ef48c99357'

_payload = _SOURCE_GZIP.read_bytes()
if hashlib.sha256(_payload).hexdigest() != _SOURCE_GZIP_SHA256:
    raise RuntimeError(f"cold-start source archive hash mismatch: {_SOURCE_GZIP}")
_source = gzip.decompress(_payload)
if hashlib.sha256(_source).hexdigest() != _SOURCE_SHA256:
    raise RuntimeError(f"cold-start source hash mismatch: {_SOURCE_GZIP}")
exec(compile(_source.decode("utf-8"), str(_SOURCE_GZIP), "exec"), globals(), globals())
