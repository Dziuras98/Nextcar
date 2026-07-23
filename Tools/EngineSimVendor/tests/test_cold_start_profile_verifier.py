#!/usr/bin/env python3
"""Mutation coverage for the Subaru EJ25 cold-start profile verifier."""

from __future__ import annotations

import hashlib
import importlib.util
import json
import math
import shutil
import tempfile
import unittest
from copy import deepcopy
from pathlib import Path
from typing import Any, Callable

ENGINE_VENDOR = Path(__file__).resolve().parents[1]


def load_module(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


generator = load_module(
    "cold_start_generator_under_test",
    ENGINE_VENDOR / "generate_cold_start_profile_header.py",
)
verifier = load_module(
    "cold_start_verifier_under_test",
    ENGINE_VENDOR / "verify_cold_start_profile.py",
)


def sha256_file(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write_json(path: Path, value: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        json.dumps(
            value,
            ensure_ascii=False,
            sort_keys=True,
            indent=2,
            allow_nan=False,
        )
        + "\n",
        encoding="utf-8",
        newline="\n",
    )


def cycle(
    index: int,
    *,
    time_seconds: float,
    rpm: float,
    starter: bool,
    disengagement: float | None,
    pcm_rms: float = 0.1,
) -> dict[str, Any]:
    return {
        "cycle_index": index,
        "simulation_time_seconds": time_seconds,
        "ignition_enabled": True,
        "starter_enabled": starter,
        "current_rpm": rpm,
        "starter_disengagement_rpm_observed": disengagement,
        "produced_native_frames": 167,
        "produced_pcm_frames": 368,
        "pcm_non_finite_samples": 0,
        "pcm_peak": 0.2,
        "pcm_rms": pcm_rms,
    }


def trace(
    *,
    candidate_id: str,
    trial_index: int,
    result: str,
    failure_reason: str,
    disengagement_time: float | None,
    disengagement_rpm: float | None,
    minimum_rpm: float,
    mean_rpm: float,
    post_min_rpm: float = 600.0,
    maximum_time: float = 4.0,
) -> dict[str, Any]:
    cycles = [
        cycle(
            0,
            time_seconds=1.0,
            rpm=disengagement_rpm or 300.0,
            starter=disengagement_time is None,
            disengagement=disengagement_rpm,
        ),
        cycle(
            1,
            time_seconds=2.0,
            rpm=minimum_rpm,
            starter=False if disengagement_time is not None else True,
            disengagement=disengagement_rpm,
        ),
    ]
    return {
        "schema_version": 1,
        "fixture_id": "SubaruEJ25AtgVideo2",
        "validated_parity_source_sha": "542d3261efc3ef48c78f337d990793aea55dd7fb",
        "engine_sim_commit": "85f7c3b959a908ed5232ede4f1a4ac7eafe6b630",
        "engine_sim_tree": "0756dea0444ada160540685dd1d28fcd3ef4aac5",
        "solver_commit": "e009f4ff1c9c4c5874e865e893cdb62e208fb2b3",
        "fixture_semantic_contract_sha256": "3ba447789024d613cdceb2382d917e9d6ce340cbeecb621d4f71133e55578f00",
        "ir_wav_sha256": "df0b8be829d28ae64e5b2818a1942a3b3e5733186bdf262cad4c2af038995d48",
        "generated_ir_sha256": "ce0702aa501d94f35b5f804efcd1b21688b9f9cacaa0fa2fc7f459c03a98e540",
        "seed": 0x4E433033,
        "candidate_id": candidate_id,
        "trial_index": trial_index,
        "startup_throttle": 0.2,
        "starter_disengagement_rpm": 800.0,
        "post_starter_minimum_rpm": post_min_rpm,
        "stability_window_seconds": 1.0,
        "maximum_startup_simulation_seconds": maximum_time,
        "starter_disengagement_time_seconds": disengagement_time,
        "starter_disengagement_rpm_observed": disengagement_rpm,
        "minimum_post_starter_rpm": minimum_rpm,
        "mean_post_starter_rpm": mean_rpm,
        "produced_native_frames": 334,
        "produced_pcm_frames": 736,
        "pcm_non_finite_samples": 0,
        "pcm_peak": 0.2,
        "pcm_rms": 0.1,
        "post_start_pcm_rms": 0.1 if disengagement_time is not None else 0.0,
        "result": result,
        "failure_reason": failure_reason,
        "cleanup": {
            "starter_disabled": True,
            "ignition_disabled": True,
            "simulator_destroyed": True,
            "fixture_destroyed": True,
        },
        "cycles": cycles,
    }


def distribution(value: float) -> dict[str, float]:
    return {
        "minimum": value,
        "maximum": value,
        "mean": value,
        "median": value,
        "p95": value,
    }


def build_repo(base: Path) -> tuple[Path, Path]:
    generator_path = base / generator.GENERATOR_PATH
    verifier_path = base / verifier.VERIFIER_PATH
    generator_path.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(
        ENGINE_VENDOR / "generate_cold_start_profile_header.py",
        generator_path,
    )
    shutil.copy2(
        ENGINE_VENDOR / "verify_cold_start_profile.py",
        verifier_path,
    )
    sweep_path = (
        base
        / "Tests/EngineSimCore/Fixtures/subaru_ej25_cold_start_sweep.json"
    )
    sweep = {
        "schema_version": 1,
        "fixture_id": "SubaruEJ25AtgVideo2",
        "startup_throttle": [0.05, 0.1, 0.2, 0.35, 0.5],
        "starter_disengagement_rpm": [500, 650, 800, 950, 1100, 1300],
        "post_starter_minimum_rpm": [400, 500, 600, 700],
        "stability_window_seconds": [0.5, 1.0, 2.0],
        "maximum_startup_simulation_seconds": [2.0, 4.0, 8.0],
        "evaluated_candidate_count": 1080,
    }
    write_json(sweep_path, sweep)

    trace_root = (
        base
        / "Tests/EngineSimCore/Fixtures/"
        "subaru_ej25_cold_start_calibration"
    )
    positive_records = []
    for index in range(10):
        path = trace_root / "positive-traces" / f"trial-{index:02d}.json"
        write_json(
            path,
            trace(
                candidate_id="synthetic-selected",
                trial_index=index,
                result="Success",
                failure_reason="",
                disengagement_time=1.0,
                disengagement_rpm=850.0,
                minimum_rpm=850.0,
                mean_rpm=850.0,
            ),
        )
        positive_records.append(
            {
                "trial_index": index,
                "path": path.relative_to(base).as_posix(),
                "sha256": sha256_file(path),
                "result": "Success",
            }
        )

    timeout_path = trace_root / "negative-timeout.json"
    write_json(
        timeout_path,
        trace(
            candidate_id="negative-timeout",
            trial_index=0,
            result="Timeout",
            failure_reason="Timeout",
            disengagement_time=None,
            disengagement_rpm=None,
            minimum_rpm=0.0,
            mean_rpm=0.0,
            maximum_time=0.25,
        ),
    )
    stall_path = trace_root / "negative-stall.json"
    write_json(
        stall_path,
        trace(
            candidate_id="negative-stall",
            trial_index=0,
            result="StallAfterDisengagement",
            failure_reason="StallAfterDisengagement",
            disengagement_time=1.0,
            disengagement_rpm=850.0,
            minimum_rpm=500.0,
            mean_rpm=675.0,
        ),
    )

    profile = {
        "schema_version": 1,
        "profile_id": "SubaruEJ25AtgVideo2ColdStartV1",
        "fixture_id": "SubaruEJ25AtgVideo2",
        "validated_parity_source_sha": "542d3261efc3ef48c78f337d990793aea55dd7fb",
        "engine_sim_commit": "85f7c3b959a908ed5232ede4f1a4ac7eafe6b630",
        "engine_sim_tree": "0756dea0444ada160540685dd1d28fcd3ef4aac5",
        "solver_commit": "e009f4ff1c9c4c5874e865e893cdb62e208fb2b3",
        "fixture_semantic_contract_sha256": "3ba447789024d613cdceb2382d917e9d6ce340cbeecb621d4f71133e55578f00",
        "ir_wav_sha256": "df0b8be829d28ae64e5b2818a1942a3b3e5733186bdf262cad4c2af038995d48",
        "generated_ir_sha256": "ce0702aa501d94f35b5f804efcd1b21688b9f9cacaa0fa2fc7f459c03a98e540",
        "simulation_frequency_hz": 20000,
        "sample_rate_hz": 44100,
        "deterministic_seed": 0x4E433033,
        "startup_throttle": 0.2,
        "starter_disengagement_criterion": {
            "type": "rpm_at_or_above",
            "rpm": 800.0,
        },
        "post_starter_minimum_rpm": 600.0,
        "stability_window_seconds": 1.0,
        "maximum_startup_simulation_seconds": 4.0,
        "candidate_sweep": {
            "path": sweep_path.relative_to(base).as_posix(),
            "sha256": sha256_file(sweep_path),
            "evaluated_candidate_count": 1080,
            "startup_throttle": [0.05, 0.1, 0.2, 0.35, 0.5],
            "starter_disengagement_rpm": [500, 650, 800, 950, 1100, 1300],
            "post_starter_minimum_rpm": [400, 500, 600, 700],
            "stability_window_seconds": [0.5, 1.0, 2.0],
            "maximum_startup_simulation_seconds": [2.0, 4.0, 8.0],
        },
        "selected_candidate_id": "synthetic-selected",
        "selected_trial_count": 10,
        "success_count": 10,
        "failure_count": 0,
        "observed_distributions": {
            "disengagement_time_seconds": distribution(1.0),
            "disengagement_rpm": distribution(850.0),
            "minimum_post_starter_rpm": distribution(850.0),
            "mean_post_starter_rpm": distribution(850.0),
            "produced_native_frames": distribution(334.0),
            "produced_pcm_frames": distribution(736.0),
            "post_start_pcm_rms": distribution(0.1),
        },
        "selected_margins": {
            "post_starter_rpm": 250.0,
            "timeout_seconds": 2.0,
            "disengagement_separation_rpm": 50.0,
        },
        "selection_requirements": {
            "minimum_selected_trials": 10,
            "required_success_fraction": 1.0,
            "minimum_post_starter_rpm_margin": 100.0,
            "minimum_timeout_margin_seconds": 1.0,
            "minimum_disengagement_separation_rpm": 1.0,
        },
        "positive_traces": positive_records,
        "timeout_trace": {
            "path": timeout_path.relative_to(base).as_posix(),
            "sha256": sha256_file(timeout_path),
            "result": "Timeout",
        },
        "stall_trace": {
            "path": stall_path.relative_to(base).as_posix(),
            "sha256": sha256_file(stall_path),
            "result": "StallAfterDisengagement",
        },
        "generator": {
            "path": generator.GENERATOR_PATH,
            "sha256": sha256_file(generator_path),
            "command": generator.GENERATOR_COMMAND,
        },
        "verifier": {
            "path": verifier.VERIFIER_PATH,
            "sha256": sha256_file(verifier_path),
        },
        "generated_header": {
            "path": generator.HEADER_PATH,
            "sha256": "",
        },
        "no_runtime_wav_dependency": True,
        "calibration_status": "windows_pass",
        "payload_sha256": "",
    }
    profile["payload_sha256"] = generator.canonical_payload_sha256(profile)
    profile_path = (
        base / "Tests/EngineSimCore/Fixtures/subaru_ej25_cold_start_profile.json"
    )
    write_json(profile_path, profile)

    header_path = base / generator.HEADER_PATH
    header_path.parent.mkdir(parents=True, exist_ok=True)
    header_path.write_bytes(generator.render_header(profile))
    profile["generated_header"]["sha256"] = sha256_file(header_path)
    write_json(profile_path, profile)
    return profile_path, header_path


def rewrite_profile(
    repo: Path,
    mutate: Callable[[dict[str, Any]], None],
    *,
    regenerate_header: bool,
) -> None:
    profile_path = (
        repo / "Tests/EngineSimCore/Fixtures/subaru_ej25_cold_start_profile.json"
    )
    profile = json.loads(profile_path.read_text(encoding="utf-8"))
    mutate(profile)
    if regenerate_header:
        profile["generated_header"]["sha256"] = ""
        profile["payload_sha256"] = generator.canonical_payload_sha256(profile)
        write_json(profile_path, profile)
        header_path = repo / generator.HEADER_PATH
        header_path.write_bytes(generator.render_header(profile))
        profile["generated_header"]["sha256"] = sha256_file(header_path)
    profile["payload_sha256"] = generator.canonical_payload_sha256(profile)
    write_json(profile_path, profile)


class ColdStartProfileVerifierTests(unittest.TestCase):
    def assertMutationRejected(
        self,
        mutator: Callable[[Path, Path, Path], None],
    ) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            repo = Path(temporary)
            profile, header = build_repo(repo)
            mutator(repo, profile, header)
            with self.assertRaises(verifier.VerificationError):
                verifier.verify(profile, repo, run_generator=False)

    def test_positive_profile(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            repo = Path(temporary)
            profile, _header = build_repo(repo)
            result = verifier.verify(profile, repo, run_generator=True)
            self.assertEqual(result["selected_trial_count"], 10)
            self.assertEqual(result["timeout_result"], "Timeout")
            self.assertEqual(result["stall_result"], "StallAfterDisengagement")

    def test_changed_startup_throttle(self) -> None:
        self.assertMutationRejected(
            lambda repo, _p, _h: rewrite_profile(
                repo,
                lambda profile: profile.__setitem__("startup_throttle", 0.1),
                regenerate_header=True,
            )
        )

    def test_changed_disengagement_criterion(self) -> None:
        self.assertMutationRejected(
            lambda repo, _p, _h: rewrite_profile(
                repo,
                lambda profile: profile["starter_disengagement_criterion"].__setitem__("rpm", 650.0),
                regenerate_header=True,
            )
        )

    def test_changed_minimum_rpm(self) -> None:
        self.assertMutationRejected(
            lambda repo, _p, _h: rewrite_profile(
                repo,
                lambda profile: profile.__setitem__("post_starter_minimum_rpm", 500.0),
                regenerate_header=True,
            )
        )

    def test_changed_stability_window(self) -> None:
        self.assertMutationRejected(
            lambda repo, _p, _h: rewrite_profile(
                repo,
                lambda profile: profile.__setitem__("stability_window_seconds", 0.5),
                regenerate_header=True,
            )
        )

    def test_changed_maximum_time(self) -> None:
        self.assertMutationRejected(
            lambda repo, _p, _h: rewrite_profile(
                repo,
                lambda profile: profile.__setitem__("maximum_startup_simulation_seconds", 8.0),
                regenerate_header=True,
            )
        )

    def test_removed_trial(self) -> None:
        self.assertMutationRejected(
            lambda repo, _p, _h: rewrite_profile(
                repo,
                lambda profile: profile["positive_traces"].pop(),
                regenerate_header=False,
            )
        )

    def test_changed_positive_trace_hash(self) -> None:
        self.assertMutationRejected(
            lambda repo, _p, _h: rewrite_profile(
                repo,
                lambda profile: profile["positive_traces"][0].__setitem__("sha256", "0" * 64),
                regenerate_header=False,
            )
        )

    def test_missing_timeout_trace(self) -> None:
        self.assertMutationRejected(
            lambda repo, _p, _h: rewrite_profile(
                repo,
                lambda profile: profile.__setitem__("timeout_trace", None),
                regenerate_header=False,
            )
        )

    def test_missing_stall_trace(self) -> None:
        self.assertMutationRejected(
            lambda repo, _p, _h: rewrite_profile(
                repo,
                lambda profile: profile.__setitem__("stall_trace", None),
                regenerate_header=False,
            )
        )

    def test_changed_fixture_hash(self) -> None:
        self.assertMutationRejected(
            lambda repo, _p, _h: rewrite_profile(
                repo,
                lambda profile: profile.__setitem__("fixture_semantic_contract_sha256", "0" * 64),
                regenerate_header=False,
            )
        )

    def test_changed_ir_hash(self) -> None:
        self.assertMutationRejected(
            lambda repo, _p, _h: rewrite_profile(
                repo,
                lambda profile: profile.__setitem__("ir_wav_sha256", "0" * 64),
                regenerate_header=False,
            )
        )

    def test_changed_generated_header_value(self) -> None:
        def mutate(_repo: Path, _profile: Path, header: Path) -> None:
            text = header.read_text(encoding="utf-8")
            header.write_text(
                text.replace(
                    "SubaruEJ25ColdStartProfileStartupThrottle =\n    0.2",
                    "SubaruEJ25ColdStartProfileStartupThrottle =\n    0.3",
                ),
                encoding="utf-8",
                newline="\n",
            )
        self.assertMutationRejected(mutate)

    def test_nan(self) -> None:
        def mutate(_repo: Path, profile: Path, _header: Path) -> None:
            text = profile.read_text(encoding="utf-8")
            profile.write_text(
                text.replace('"startup_throttle": 0.2', '"startup_throttle": NaN'),
                encoding="utf-8",
                newline="\n",
            )
        self.assertMutationRejected(mutate)

    def test_infinity(self) -> None:
        def mutate(_repo: Path, profile: Path, _header: Path) -> None:
            text = profile.read_text(encoding="utf-8")
            profile.write_text(
                text.replace('"startup_throttle": 0.2', '"startup_throttle": Infinity'),
                encoding="utf-8",
                newline="\n",
            )
        self.assertMutationRejected(mutate)


if __name__ == "__main__":
    unittest.main()
