#!/usr/bin/env python3
"""Independently verify a pinned WAV, generated PCM header, and manifest linkage."""

from __future__ import annotations

import argparse
import ast
import hashlib
import io
import json
from pathlib import Path
import re
import struct
import sys
import wave

EXPECTED_SAMPLE_RATE = 44_100
EXPECTED_CHANNELS = 1
EXPECTED_BITS_PER_SAMPLE = 16
EXPECTED_AUDIO_FORMAT = 1
EXPECTED_NOTICE = "ThirdPartyNotices/engine-sim.txt"
EXPECTED_GENERATOR = "Tools/EngineSimVendor/generate_impulse_response_header.py"
EXPECTED_GENERATION_COMMAND = (
    "python3 Tools/EngineSimVendor/generate_impulse_response_header.py "
    "--input Plugins/NextcarEngineSim/Source/ThirdParty/EngineSim/upstream/engine-sim/"
    "es/sound-library/new/minimal_muffling_02.wav "
    "--output Plugins/NextcarEngineSim/Source/NextcarEngineSimCore/Private/Generated/"
    "MinimalMuffling02ImpulseResponse.generated.h"
)


class VerificationError(ValueError):
    pass


def _sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def _git_blob_sha(data: bytes) -> str:
    return hashlib.sha1(f"blob {len(data)}\0".encode("ascii") + data).hexdigest()


def _raw_wav_fields(data: bytes) -> dict[str, object]:
    if len(data) < 12 or data[0:4] != b"RIFF" or data[8:12] != b"WAVE":
        raise VerificationError("raw RIFF/WAVE signature is invalid")
    riff_size = struct.unpack_from("<I", data, 4)[0]
    if riff_size + 8 != len(data):
        raise VerificationError("raw RIFF size does not match file length")

    chunks: list[tuple[bytes, bytes]] = []
    cursor = 12
    while cursor < len(data):
        if cursor + 8 > len(data):
            raise VerificationError("truncated raw RIFF chunk header")
        chunk_id = data[cursor : cursor + 4]
        chunk_size = struct.unpack_from("<I", data, cursor + 4)[0]
        begin = cursor + 8
        end = begin + chunk_size
        padded_end = end + (chunk_size % 2)
        if end > len(data) or padded_end > len(data):
            raise VerificationError(f"raw chunk {chunk_id!r} exceeds file length")
        chunks.append((chunk_id, data[begin:end]))
        cursor = padded_end
    if cursor != len(data):
        raise VerificationError("raw RIFF traversal did not terminate at EOF")

    fmt_chunks = [payload for chunk_id, payload in chunks if chunk_id == b"fmt "]
    data_chunks = [payload for chunk_id, payload in chunks if chunk_id == b"data"]
    if len(fmt_chunks) != 1 or len(data_chunks) != 1:
        raise VerificationError("exactly one fmt chunk and one data chunk are required")
    fmt = fmt_chunks[0]
    if len(fmt) < 16:
        raise VerificationError("raw fmt chunk is too short")
    audio_format, channels, rate, byte_rate, align, bits = struct.unpack_from("<HHIIHH", fmt, 0)
    pcm = data_chunks[0]
    if align == 0 or len(pcm) % align:
        raise VerificationError("raw data chunk has incomplete frames")
    return {
        "audio_format": audio_format,
        "channels": channels,
        "sample_rate": rate,
        "byte_rate": byte_rate,
        "block_align": align,
        "bits_per_sample": bits,
        "data_size": len(pcm),
        "sample_count": len(pcm) // align,
        "pcm": pcm,
    }


def _wave_module_fields(data: bytes) -> dict[str, object]:
    try:
        with wave.open(io.BytesIO(data), "rb") as reader:
            channels = reader.getnchannels()
            width = reader.getsampwidth()
            rate = reader.getframerate()
            frames = reader.getnframes()
            compression = reader.getcomptype()
            pcm = reader.readframes(frames)
            trailing = reader.readframes(1)
    except (wave.Error, EOFError) as exc:
        raise VerificationError(f"wave module rejected WAV: {exc}") from exc
    if compression != "NONE":
        raise VerificationError(f"wave module reports unsupported compression {compression}")
    if trailing:
        raise VerificationError("wave module returned frames beyond declared sample count")
    return {
        "channels": channels,
        "bits_per_sample": width * 8,
        "sample_rate": rate,
        "sample_count": frames,
        "pcm": pcm,
    }


def _extract_integer(text: str, name: str) -> int:
    match = re.search(rf"\b{name}\s*=\s*(\d+)\s*;", text)
    if not match:
        raise VerificationError(f"generated header is missing {name}")
    return int(match.group(1))


def _extract_string(text: str, name: str) -> str:
    match = re.search(rf'\b{name}\[\]\s*=\s*"([0-9a-f]+)"\s*;', text)
    if not match:
        raise VerificationError(f"generated header is missing {name}")
    return match.group(1)


def _header_fields(data: bytes) -> dict[str, object]:
    try:
        text = data.decode("utf-8")
    except UnicodeDecodeError as exc:
        raise VerificationError("generated header is not UTF-8") from exc
    match = re.search(
        r"MinimalMuffling02Pcm\s*\[\]\s*=\s*\{(?P<body>.*?)\};", text, re.DOTALL
    )
    if not match:
        raise VerificationError("generated header is missing MinimalMuffling02Pcm")
    body = match.group("body")
    tokens = [token.strip() for token in body.split(",") if token.strip()]
    samples: list[int] = []
    for token in tokens:
        try:
            value = ast.literal_eval(token)
        except (ValueError, SyntaxError) as exc:
            raise VerificationError(f"invalid PCM literal {token!r}") from exc
        if not isinstance(value, int) or value < -32768 or value > 32767:
            raise VerificationError(f"PCM literal is outside int16 range: {token!r}")
        samples.append(value)
    pcm = struct.pack(f"<{len(samples)}h", *samples) if samples else b""
    return {
        "samples": tuple(samples),
        "pcm": pcm,
        "sample_count": _extract_integer(text, "MinimalMuffling02SampleCount"),
        "byte_count": _extract_integer(text, "MinimalMuffling02ByteCount"),
        "sample_rate": _extract_integer(text, "MinimalMuffling02SampleRate"),
        "channels": _extract_integer(text, "MinimalMuffling02Channels"),
        "bits_per_sample": _extract_integer(text, "MinimalMuffling02BitsPerSample"),
        "pcm_sha256": _extract_string(text, "MinimalMuffling02PcmSha256"),
        "wav_sha256": _extract_string(text, "MinimalMuffling02WavSha256"),
        "git_blob_sha": _extract_string(text, "MinimalMuffling02SourceGitBlobSha"),
    }


def _require(condition: bool, message: str) -> None:
    if not condition:
        raise VerificationError(message)


def _find_entry(
    files: list[dict[str, object]],
    *,
    status: str,
    identifying_field: str,
    identifying_value: str,
) -> dict[str, object]:
    matches = [
        entry
        for entry in files
        if entry.get("status") == status
        and entry.get(identifying_field) == identifying_value
    ]
    if len(matches) != 1:
        raise VerificationError(
            f"manifest must contain exactly one {status!r} IR entry with "
            f"{identifying_field}={identifying_value!r}"
        )
    return matches[0]


def verify(wav_path: Path, header_path: Path, manifest_path: Path, repository_root: Path) -> None:
    wav_data = wav_path.read_bytes()
    header_data = header_path.read_bytes()
    raw = _raw_wav_fields(wav_data)
    via_wave = _wave_module_fields(wav_data)
    header = _header_fields(header_data)

    _require(raw["audio_format"] == EXPECTED_AUDIO_FORMAT, "WAV format code is not PCM 1")
    _require(raw["channels"] == EXPECTED_CHANNELS, "WAV is not mono")
    _require(raw["sample_rate"] == EXPECTED_SAMPLE_RATE, "WAV sample rate is not 44100 Hz")
    _require(raw["bits_per_sample"] == EXPECTED_BITS_PER_SAMPLE, "WAV is not PCM16")
    _require(raw["block_align"] == 2, "WAV block alignment is not 2")
    _require(raw["byte_rate"] == 88_200, "WAV byte rate is not 88200")
    for key in ("channels", "sample_rate", "bits_per_sample", "sample_count", "pcm"):
        _require(raw[key] == via_wave[key], f"raw RIFF and wave module disagree on {key}")

    pcm = raw["pcm"]
    sample_count = int(raw["sample_count"])
    wav_sha = _sha256(wav_data)
    pcm_sha = _sha256(pcm)  # type: ignore[arg-type]
    blob_sha = _git_blob_sha(wav_data)
    header_sha = _sha256(header_data)

    _require(len(header["samples"]) == sample_count, "header PCM literal count differs from WAV")
    _require(header["sample_count"] == sample_count, "header sample count constant differs from WAV")
    _require(header["byte_count"] == len(pcm), "header byte count differs from WAV data chunk")  # type: ignore[arg-type]
    _require(header["sample_rate"] == EXPECTED_SAMPLE_RATE, "header sample rate differs")
    _require(header["channels"] == EXPECTED_CHANNELS, "header channel count differs")
    _require(header["bits_per_sample"] == EXPECTED_BITS_PER_SAMPLE, "header bit depth differs")
    _require(header["pcm"] == pcm, "one or more generated PCM samples differ from the WAV")
    _require(header["pcm_sha256"] == pcm_sha, "header PCM SHA-256 constant differs")
    _require(header["wav_sha256"] == wav_sha, "header WAV SHA-256 constant differs")
    _require(header["git_blob_sha"] == blob_sha, "header Git blob SHA constant differs")

    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    files = manifest.get("files")
    if not isinstance(files, list):
        raise VerificationError("manifest files field is not a list")
    wav_entry = _find_entry(
        files,
        status="copied",
        identifying_field="upstream_path",
        identifying_value="es/sound-library/new/minimal_muffling_02.wav",
    )
    header_entry = _find_entry(
        files,
        status="generated",
        identifying_field="destination_path",
        identifying_value=(
            "Plugins/NextcarEngineSim/Source/NextcarEngineSimCore/Private/Generated/"
            "MinimalMuffling02ImpulseResponse.generated.h"
        ),
    )

    required_wav = {
        "repository": "ange-yaghi/engine-sim",
        "pinned_fork": "Dziuras98/engine-sim",
        "engine_sim_commit": "85f7c3b959a908ed5232ede4f1a4ac7eafe6b630",
        "introduction_commit": "4f7e06b211d0b51914aed0539b397ac27f70d0f3",
        "upstream_path": "es/sound-library/new/minimal_muffling_02.wav",
        "git_blob_sha": blob_sha,
        "wav_sha256": wav_sha,
        "format": "PCM signed 16-bit little-endian mono",
        "sample_rate": EXPECTED_SAMPLE_RATE,
        "channels": EXPECTED_CHANNELS,
        "bits_per_sample": EXPECTED_BITS_PER_SAMPLE,
        "sample_count": sample_count,
        "data_chunk_size": len(pcm),  # type: ignore[arg-type]
        "pcm_sha256": pcm_sha,
        "byte_for_byte_copy_verified": True,
        "license": "MIT",
        "notice_mapping": EXPECTED_NOTICE,
    }
    for key, value in required_wav.items():
        _require(wav_entry.get(key) == value, f"manifest copied WAV field {key!r} differs")
    _require(isinstance(wav_entry.get("source_input_path"), str), "manifest lacks source_input_path")
    _require(isinstance(wav_entry.get("vendored_path"), str), "manifest lacks vendored_path")

    generator_path = repository_root / EXPECTED_GENERATOR
    generator_sha = _sha256(generator_path.read_bytes())
    required_generated = {
        "generator_path": EXPECTED_GENERATOR,
        "generator_sha256": generator_sha,
        "generation_command": EXPECTED_GENERATION_COMMAND,
        "source_git_blob_sha": blob_sha,
        "source_wav_sha256": wav_sha,
        "generated_header_sha256": header_sha,
        "sample_rate": EXPECTED_SAMPLE_RATE,
        "channels": EXPECTED_CHANNELS,
        "bits_per_sample": EXPECTED_BITS_PER_SAMPLE,
        "sample_count": sample_count,
        "pcm_sha256": pcm_sha,
        "license": "MIT",
        "notice_mapping": EXPECTED_NOTICE,
    }
    for key, value in required_generated.items():
        _require(header_entry.get(key) == value, f"manifest generated header field {key!r} differs")
    _require(isinstance(header_entry.get("destination_path"), str), "manifest lacks destination_path")
    _require(header_entry.get("source_entry") == wav_entry.get("id"), "generated entry is not linked to WAV entry")
    _require((repository_root / EXPECTED_NOTICE).is_file(), "mapped engine-sim notice file is missing")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--wav", required=True, type=Path)
    parser.add_argument("--header", required=True, type=Path)
    parser.add_argument("--manifest", required=True, type=Path)
    parser.add_argument(
        "--repository-root",
        type=Path,
        default=Path.cwd(),
        help="repository root used to validate generator and notice paths (default: cwd)",
    )
    args = parser.parse_args(argv)
    try:
        verify(args.wav, args.header, args.manifest, args.repository_root.resolve())
    except (OSError, VerificationError, json.JSONDecodeError, struct.error) as exc:
        print(f"verification failed: {exc}", file=sys.stderr)
        return 1
    print("impulse-response verification passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
