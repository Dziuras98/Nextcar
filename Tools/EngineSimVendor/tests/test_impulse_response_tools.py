#!/usr/bin/env python3
"""Tests for the deterministic impulse-response generator and independent verifier."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
import wave

REPOSITORY_ROOT = Path(__file__).resolve().parents[3]
GENERATOR = REPOSITORY_ROOT / "Tools/EngineSimVendor/generate_impulse_response_header.py"
VERIFIER = REPOSITORY_ROOT / "Tools/EngineSimVendor/verify_impulse_response.py"
GENERATOR_RELATIVE = "Tools/EngineSimVendor/generate_impulse_response_header.py"
NOTICE_RELATIVE = "ThirdPartyNotices/engine-sim.txt"
GENERATION_COMMAND = (
    "python3 Tools/EngineSimVendor/generate_impulse_response_header.py "
    "--input Plugins/NextcarEngineSim/Source/ThirdParty/EngineSim/upstream/engine-sim/"
    "es/sound-library/new/minimal_muffling_02.wav "
    "--output Plugins/NextcarEngineSim/Source/NextcarEngineSimCore/Private/Generated/"
    "MinimalMuffling02ImpulseResponse.generated.h"
)
PINNED_ENGINE_SIM_COMMIT = "85f7c3b959a908ed5232ede4f1a4ac7eafe6b630"
INTRODUCTION_COMMIT = "4f7e06b211d0b51914aed0539b397ac27f70d0f3"
UPSTREAM_PATH = "es/sound-library/new/minimal_muffling_02.wav"


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def git_blob_sha(data: bytes) -> str:
    return hashlib.sha1(f"blob {len(data)}\0".encode("ascii") + data).hexdigest()


def run_tool(*arguments: object, cwd: Path | None = None) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [sys.executable, *(str(argument) for argument in arguments)],
        cwd=cwd,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )


def create_wave_file(
    path: Path,
    samples: tuple[int, ...] = (0, 32767, -32768, 1234, -2345, 1, -1),
    *,
    channels: int = 1,
    sample_width: int = 2,
    sample_rate: int = 44_100,
) -> bytes:
    path.parent.mkdir(parents=True, exist_ok=True)
    if sample_width == 2:
        values = samples * channels if channels > 1 else samples
        frames = struct.pack(f"<{len(values)}h", *values)
    elif sample_width == 1:
        values = tuple((sample + 128) & 0xFF for sample in samples)
        frames = bytes(values * channels if channels > 1 else values)
    else:
        raise AssertionError("unsupported test width")
    with wave.open(str(path), "wb") as writer:
        writer.setnchannels(channels)
        writer.setsampwidth(sample_width)
        writer.setframerate(sample_rate)
        writer.writeframes(frames)
    return path.read_bytes()


def add_extra_chunk(wav_data: bytes, chunk_id: bytes = b"JUNK", payload: bytes = b"abc") -> bytes:
    if len(chunk_id) != 4:
        raise AssertionError("test chunk id must be four bytes")
    encoded = chunk_id + struct.pack("<I", len(payload)) + payload
    if len(payload) & 1:
        encoded += b"\0"
    result = bytearray(wav_data[:12] + encoded + wav_data[12:])
    struct.pack_into("<I", result, 4, len(result) - 8)
    return bytes(result)


def parse_pcm(wav_data: bytes) -> tuple[bytes, int]:
    cursor = 12
    while cursor < len(wav_data):
        size = struct.unpack_from("<I", wav_data, cursor + 4)[0]
        begin = cursor + 8
        end = begin + size
        if wav_data[cursor : cursor + 4] == b"data":
            return wav_data[begin:end], size // 2
        cursor = end + (size & 1)
    raise AssertionError("test WAV lacks data chunk")


class ImpulseResponseToolTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.wav = self.root / "input.wav"
        self.header = self.root / "generated.h"
        self.header_second = self.root / "generated-second.h"
        self.manifest = self.root / "manifest.json"
        generator_copy = self.root / GENERATOR_RELATIVE
        generator_copy.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(GENERATOR, generator_copy)
        notice = self.root / NOTICE_RELATIVE
        notice.parent.mkdir(parents=True, exist_ok=True)
        notice.write_text("Synthetic test notice.\n", encoding="utf-8")
        create_wave_file(self.wav)

    def tearDown(self) -> None:
        self.temp.cleanup()

    def generate(self, wav: Path | None = None, output: Path | None = None) -> subprocess.CompletedProcess[str]:
        return run_tool(GENERATOR, "--input", wav or self.wav, "--output", output or self.header)

    def write_manifest(self, wav: Path | None = None, header: Path | None = None) -> None:
        wav_path = wav or self.wav
        header_path = header or self.header
        wav_data = wav_path.read_bytes()
        header_data = header_path.read_bytes()
        pcm, sample_count = parse_pcm(wav_data)
        copied_id = "minimal-muffling-02-wav"
        payload = {
            "schema_version": 1,
            "files": [
                {
                    "id": copied_id,
                    "status": "copied",
                    "repository": "ange-yaghi/engine-sim",
                    "pinned_fork": "Dziuras98/engine-sim",
                    "engine_sim_commit": PINNED_ENGINE_SIM_COMMIT,
                    "introduction_commit": INTRODUCTION_COMMIT,
                    "upstream_path": UPSTREAM_PATH,
                    "git_blob_sha": git_blob_sha(wav_data),
                    "source_input_path": "Plugins/NextcarEngineSim/Source/ThirdParty/EngineSim/upstream/engine-sim/" + UPSTREAM_PATH,
                    "vendored_path": "Plugins/NextcarEngineSim/Source/ThirdParty/EngineSim/upstream/engine-sim/" + UPSTREAM_PATH,
                    "wav_sha256": sha256(wav_data),
                    "format": "PCM signed 16-bit little-endian mono",
                    "sample_rate": 44_100,
                    "channels": 1,
                    "bits_per_sample": 16,
                    "sample_count": sample_count,
                    "data_chunk_size": len(pcm),
                    "pcm_sha256": sha256(pcm),
                    "byte_for_byte_copy_verified": True,
                    "license": "MIT",
                    "notice_mapping": NOTICE_RELATIVE,
                },
                {
                    "id": "minimal-muffling-02-header",
                    "status": "generated",
                    "destination_path": "Plugins/NextcarEngineSim/Source/NextcarEngineSimCore/Private/Generated/MinimalMuffling02ImpulseResponse.generated.h",
                    "generator_path": GENERATOR_RELATIVE,
                    "generator_sha256": sha256((self.root / GENERATOR_RELATIVE).read_bytes()),
                    "generation_command": GENERATION_COMMAND,
                    "source_entry": copied_id,
                    "source_git_blob_sha": git_blob_sha(wav_data),
                    "source_wav_sha256": sha256(wav_data),
                    "generated_header_sha256": sha256(header_data),
                    "sample_rate": 44_100,
                    "channels": 1,
                    "bits_per_sample": 16,
                    "sample_count": sample_count,
                    "pcm_sha256": sha256(pcm),
                    "license": "MIT",
                    "notice_mapping": NOTICE_RELATIVE,
                },
            ],
        }
        self.manifest.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")

    def verify(
        self,
        wav: Path | None = None,
        header: Path | None = None,
        manifest: Path | None = None,
    ) -> subprocess.CompletedProcess[str]:
        return run_tool(
            VERIFIER,
            "--wav",
            wav or self.wav,
            "--header",
            header or self.header,
            "--manifest",
            manifest or self.manifest,
            "--repository-root",
            self.root,
        )

    def prepare_valid_fixture(self) -> None:
        result = self.generate()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.write_manifest()

    def assert_generator_rejects(self, wav: Path) -> None:
        result = self.generate(wav, self.root / "rejected.h")
        self.assertNotEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertFalse((self.root / "rejected.h").exists())

    def test_two_generations_are_byte_for_byte_deterministic_and_verify(self) -> None:
        first = self.generate()
        second = self.generate(output=self.header_second)
        self.assertEqual(first.returncode, 0, first.stderr)
        self.assertEqual(second.returncode, 0, second.stderr)
        self.assertEqual(self.header.read_bytes(), self.header_second.read_bytes())
        self.write_manifest()
        verified = self.verify()
        self.assertEqual(verified.returncode, 0, verified.stderr)
        self.assertIn("verification passed", verified.stdout)

    def test_verifier_rejects_changed_pcm_sample(self) -> None:
        self.prepare_valid_fixture()
        text = self.header.read_text(encoding="utf-8")
        changed = text.replace("    0, 32767,", "    1, 32767,", 1)
        self.assertNotEqual(text, changed)
        altered = self.root / "altered-sample.h"
        altered.write_text(changed, encoding="utf-8")
        result = self.verify(header=altered)
        self.assertNotEqual(result.returncode, 0)

    def test_verifier_rejects_changed_sample_count(self) -> None:
        self.prepare_valid_fixture()
        text = self.header.read_text(encoding="utf-8")
        altered = self.root / "altered-count.h"
        altered.write_text(
            text.replace("MinimalMuffling02SampleCount = 7;", "MinimalMuffling02SampleCount = 8;", 1),
            encoding="utf-8",
        )
        result = self.verify(header=altered)
        self.assertNotEqual(result.returncode, 0)

    def test_verifier_rejects_changed_sample_rate(self) -> None:
        self.prepare_valid_fixture()
        text = self.header.read_text(encoding="utf-8")
        altered = self.root / "altered-rate.h"
        altered.write_text(
            text.replace("MinimalMuffling02SampleRate = 44100;", "MinimalMuffling02SampleRate = 48000;", 1),
            encoding="utf-8",
        )
        result = self.verify(header=altered)
        self.assertNotEqual(result.returncode, 0)

    def test_verifier_rejects_corrupted_wav(self) -> None:
        self.prepare_valid_fixture()
        data = bytearray(self.wav.read_bytes())
        data[44] ^= 0x01
        corrupted = self.root / "corrupted.wav"
        corrupted.write_bytes(data)
        result = self.verify(wav=corrupted)
        self.assertNotEqual(result.returncode, 0)

    def test_generator_and_verifier_reject_truncated_wav(self) -> None:
        self.prepare_valid_fixture()
        truncated = self.root / "truncated.wav"
        truncated.write_bytes(self.wav.read_bytes()[:-1])
        self.assert_generator_rejects(truncated)
        result = self.verify(wav=truncated)
        self.assertNotEqual(result.returncode, 0)

    def test_generator_rejects_stereo(self) -> None:
        stereo = self.root / "stereo.wav"
        create_wave_file(stereo, channels=2)
        self.assert_generator_rejects(stereo)

    def test_generator_rejects_pcm8(self) -> None:
        pcm8 = self.root / "pcm8.wav"
        create_wave_file(pcm8, sample_width=1)
        self.assert_generator_rejects(pcm8)

    def test_generator_rejects_wrong_sample_rate(self) -> None:
        wrong_rate = self.root / "wrong-rate.wav"
        create_wave_file(wrong_rate, sample_rate=48_000)
        self.assert_generator_rejects(wrong_rate)

    def test_unknown_chunk_with_odd_length_and_padding_is_supported(self) -> None:
        extra = self.root / "extra-chunk.wav"
        extra_header = self.root / "extra.h"
        extra.write_bytes(add_extra_chunk(self.wav.read_bytes(), payload=b"abc"))
        result = self.generate(extra, extra_header)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.write_manifest(extra, extra_header)
        verified = self.verify(extra, extra_header)
        self.assertEqual(verified.returncode, 0, verified.stderr)

    def test_invalid_chunk_length_is_rejected(self) -> None:
        invalid = bytearray(add_extra_chunk(self.wav.read_bytes(), payload=b"abc"))
        struct.pack_into("<I", invalid, 16, len(invalid) + 100)
        path = self.root / "invalid-chunk.wav"
        path.write_bytes(invalid)
        self.assert_generator_rejects(path)


if __name__ == "__main__":
    unittest.main()
