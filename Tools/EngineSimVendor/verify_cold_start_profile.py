#!/usr/bin/env python3
"""Integrity-checked launcher for the deterministic cold-start calibration source."""
from __future__ import annotations

import gzip
import hashlib
from pathlib import Path

_SOURCE_GZIP = Path(__file__).resolve().parent / '_cold_start_sources/verify_cold_start_profile.py.gz'
_SOURCE_GZIP_SHA256 = '43e1953bfb4ace2034a4e2328b472a6d21af2575923442aac3081158e804dc10'
_SOURCE_SHA256 = '92b4ebdb922b409cf8c583fc7c7d7cb56a16dd70606c2080673ef6dc77e7d57d'

_payload = _SOURCE_GZIP.read_bytes()
if hashlib.sha256(_payload).hexdigest() != _SOURCE_GZIP_SHA256:
    raise RuntimeError(f"cold-start source archive hash mismatch: {_SOURCE_GZIP}")
_source = gzip.decompress(_payload)
if hashlib.sha256(_source).hexdigest() != _SOURCE_SHA256:
    raise RuntimeError(f"cold-start source hash mismatch: {_SOURCE_GZIP}")
exec(compile(_source.decode("utf-8"), str(_SOURCE_GZIP), "exec"), globals(), globals())
