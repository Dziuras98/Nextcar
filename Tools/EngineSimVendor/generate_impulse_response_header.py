#!/usr/bin/env python3
"""Generate the deterministic MinimalMuffling02 PCM C++ header from a WAV file."""

from __future__ import annotations

import argparse
import hashlib
import os
from pathlib import Path
import struct
import sys
import tempfile
from dataclasses import dataclass

EXPECTED_SAMPLE_RATE = 44_100
EXPECTED_CHANNELS = 1
EXPECTED_BITS_PER_SAMPLE = 16
EXPECTED_AUDIO_FORMAT = 1
UPSTREAM_PATH = "es/sound-library/new/minimal_muffling_02.wav"
GENERATOR_PATH = "Tools/EngineSimVendor/generate_impulse_response_header.py"


class WavFormatError(ValueError):
    pass


@dataclass(frozen=True)
class ParsedWav:
    file_bytes: bytes
    pcm_bytes: bytes
    samples: tuple[int, ...]
    sample_rate: int
    channels: int
    bits_per_sample: int
    block_align: int
    byte_rate: int
    audio_format: int


def _git_blob_sha(data: bytes) -> str:
    prefix = f"blob {len(data)}\0".encode("ascii")
    return hashlib.sha1(prefix + data).hexdigest()


def _parse_riff(data: bytes) -> ParsedWav:
    if len(data) < 12:
        raise WavFormatError("file is shorter than a RIFF/WAVE header")
    if data[:4] != b"RIFF" or data[8:12] != b"WAVE":
        raise WavFormatError("input is not a little-endian RIFF/WAVE file")

    declared_size = struct.unpack_from("<I", data, 4)[0]
    if declared_size + 8 != len(data):
        raise WavFormatError(
            f"RIFF size mismatch: header declares {declared_size + 8} bytes, file has {len(data)}"
        )

    fmt_payload: bytes | None = None
    pcm_payload: bytes | None = None
    offset = 12
    while offset < len(data):
        if len(data) - offset < 8:
            raise WavFormatError(f"truncated chunk header at byte {offset}")
        chunk_id = data[offset : offset + 4]
        chunk_size = struct.unpack_from("<I", data, offset + 4)[0]
        payload_start = offset + 8
        payload_end = payload_start + chunk_size
        padded_end = payload_end + (chunk_size & 1)
        if payload_end > len(data):
            raise WavFormatError(
                f"chunk {chunk_id!r} declares {chunk_size} bytes beyond end of file"
            )
        if padded_end > len(data):
            raise WavFormatError(f"chunk {chunk_id!r} is missing its RIFF padding byte")

        payload = data[payload_start:payload_end]
        if chunk_id == b"fmt ":
            if fmt_payload is not None:
                raise WavFormatError("multiple fmt chunks are not supported")
            fmt_payload = payload
        elif chunk_id == b"data":
            if pcm_payload is not None:
                raise WavFormatError("multiple data chunks are not supported")
            pcm_payload = payload
        offset = padded_end

    if offset != len(data):
        raise WavFormatError("RIFF chunk traversal did not end at the file boundary")
    if fmt_payload is None:
        raise WavFormatError("missing fmt chunk")
    if pcm_payload is None:
        raise WavFormatError("missing data chunk")
    if len(fmt_payload) < 16:
        raise WavFormatError("fmt chunk is shorter than the PCM base format")

    audio_format, channels, sample_rate, byte_rate, block_align, bits_per_sample = struct.unpack_from(
        "<HHIIHH", fmt_payload, 0
    )
    if audio_format != EXPECTED_AUDIO_FORMAT:
        raise WavFormatError(f"unsupported WAV audio format code {audio_format}; PCM code 1 required")
    if channels != EXPECTED_CHANNELS:
        raise WavFormatError(f"unsupported channel count {channels}; mono required")
    if sample_rate != EXPECTED_SAMPLE_RATE:
        raise WavFormatError(
            f"unsupported sample rate {sample_rate}; {EXPECTED_SAMPLE_RATE} Hz required"
        )
    if bits_per_sample != EXPECTED_BITS_PER_SAMPLE:
        raise WavFormatError(
            f"unsupported bit depth {bits_per_sample}; signed PCM16 little-endian required"
        )
    expected_block_align = channels * (bits_per_sample // 8)
    if block_align != expected_block_align:
        raise WavFormatError(
            f"invalid block alignment {block_align}; expected {expected_block_align}"
        )
    expected_byte_rate = sample_rate * block_align
    if byte_rate != expected_byte_rate:
        raise WavFormatError(f"invalid byte rate {byte_rate}; expected {expected_byte_rate}")
    if len(pcm_payload) % block_align != 0:
        raise WavFormatError("data chunk is not aligned to complete PCM frames")

    sample_count = len(pcm_payload) // 2
    samples = struct.unpack(f"<{sample_count}h", pcm_payload) if sample_count else ()
    return ParsedWav(
        file_bytes=data,
        pcm_bytes=pcm_payload,
        samples=tuple(samples),
        sample_rate=sample_rate,
        channels=channels,
        bits_per_sample=bits_per_sample,
        block_align=block_align,
        byte_rate=byte_rate,
        audio_format=audio_format,
    )


def _format_samples(samples: tuple[int, ...]) -> str:
    if not samples:
        return ""
    rows = []
    width = 12
    for index in range(0, len(samples), width):
        rows.append("    " + ", ".join(str(value) for value in samples[index : index + width]) + ",")
    return "\n".join(rows)


def _render_header(parsed: ParsedWav) -> bytes:
    wav_sha256 = hashlib.sha256(parsed.file_bytes).hexdigest()
    pcm_sha256 = hashlib.sha256(parsed.pcm_bytes).hexdigest()
    git_blob_sha = _git_blob_sha(parsed.file_bytes)
    body = f"""// Generated deterministically. Do not edit.
// Upstream path: {UPSTREAM_PATH}
// Source Git blob SHA: {git_blob_sha}
// Source WAV SHA-256: {wav_sha256}
// Generator: {GENERATOR_PATH}
#pragma once

#include <cstddef>
#include <cstdint>

namespace NextcarEngineSim::Generated {{

inline constexpr std::int16_t MinimalMuffling02Pcm[] = {{
{_format_samples(parsed.samples)}
}};
inline constexpr std::size_t MinimalMuffling02SampleCount = {len(parsed.samples)};
inline constexpr std::size_t MinimalMuffling02ByteCount = {len(parsed.pcm_bytes)};
inline constexpr std::uint32_t MinimalMuffling02SampleRate = {parsed.sample_rate};
inline constexpr std::uint16_t MinimalMuffling02Channels = {parsed.channels};
inline constexpr std::uint16_t MinimalMuffling02BitsPerSample = {parsed.bits_per_sample};
inline constexpr char MinimalMuffling02PcmSha256[] = "{pcm_sha256}";
inline constexpr char MinimalMuffling02WavSha256[] = "{wav_sha256}";
inline constexpr char MinimalMuffling02SourceGitBlobSha[] = "{git_blob_sha}";

static_assert(MinimalMuffling02SampleCount ==
    sizeof(MinimalMuffling02Pcm) / sizeof(MinimalMuffling02Pcm[0]));
static_assert(MinimalMuffling02ByteCount ==
    MinimalMuffling02SampleCount * sizeof(MinimalMuffling02Pcm[0]));

}}  // namespace NextcarEngineSim::Generated
"""
    return body.encode("utf-8")


def _atomic_write(path: Path, data: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temp_name: str | None = None
    try:
        with tempfile.NamedTemporaryFile(
            mode="wb", dir=path.parent, prefix=f".{path.name}.", suffix=".tmp", delete=False
        ) as handle:
            temp_name = handle.name
            handle.write(data)
            handle.flush()
            os.fsync(handle.fileno())
        os.replace(temp_name, path)
        temp_name = None
    finally:
        if temp_name is not None:
            try:
                os.unlink(temp_name)
            except FileNotFoundError:
                pass


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", required=True, type=Path, help="input RIFF/WAVE PCM file")
    parser.add_argument("--output", required=True, type=Path, help="generated C++ header")
    args = parser.parse_args(argv)

    try:
        source = args.input.read_bytes()
        parsed = _parse_riff(source)
        _atomic_write(args.output, _render_header(parsed))
    except (OSError, WavFormatError, struct.error) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
