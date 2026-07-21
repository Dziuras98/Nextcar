#!/usr/bin/env python3
"""Integrity-checked launcher for the deterministic cold-start calibration source."""
from __future__ import annotations

import gzip
import hashlib
from pathlib import Path

_SOURCE_GZIP = Path(__file__).resolve().parent / '_cold_start_sources/generate_cold_start_profile_header.py.gz'
_SOURCE_GZIP_SHA256 = '07214bae5ba90ab79bbe95eb51c650894b4c860b95181c8bbe83e2a19faf1f6a'
_SOURCE_SHA256 = '6efb3d29734c2d41c96d16e914ab460cd212769d611ba08cf2677377d933659d'

_payload = _SOURCE_GZIP.read_bytes()
if hashlib.sha256(_payload).hexdigest() != _SOURCE_GZIP_SHA256:
    raise RuntimeError(f"cold-start source archive hash mismatch: {_SOURCE_GZIP}")
_source = gzip.decompress(_payload)
if hashlib.sha256(_source).hexdigest() != _SOURCE_SHA256:
    raise RuntimeError(f"cold-start source hash mismatch: {_SOURCE_GZIP}")
exec(compile(_source.decode("utf-8"), str(_SOURCE_GZIP), "exec"), globals(), globals())
