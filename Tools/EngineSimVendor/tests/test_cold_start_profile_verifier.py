#!/usr/bin/env python3
"""Integrity-checked launcher for the cold-start verifier mutation suite."""
from __future__ import annotations

import gzip
import hashlib
from pathlib import Path

_SOURCE_GZIP = Path(__file__).resolve().parent / "../_cold_start_sources/test_cold_start_profile_verifier.py.gz"
_SOURCE_GZIP_SHA256 = '71478e6197ef589827ba0811575610c81268b2d85cb3dc0b60884d3af886f119'
_SOURCE_SHA256 = '063b6974aa2417e53b70cbcd73f0d54c4109528624c582e1350e72b68291e751'

_payload = _SOURCE_GZIP.read_bytes()
if hashlib.sha256(_payload).hexdigest() != _SOURCE_GZIP_SHA256:
    raise RuntimeError(f"cold-start test source archive hash mismatch: {_SOURCE_GZIP}")
_source = gzip.decompress(_payload)
if hashlib.sha256(_source).hexdigest() != _SOURCE_SHA256:
    raise RuntimeError(f"cold-start test source hash mismatch: {_SOURCE_GZIP}")
exec(compile(_source.decode("utf-8"), str(_SOURCE_GZIP), "exec"), globals(), globals())
