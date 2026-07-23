#!/usr/bin/env python3
"""Independently verify the measured Subaru EJ25 cold-start profile and header."""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import re
import statistics
import subprocess
import sys
import tempfile
from copy import deepcopy
from pathlib import Path
from typing import Any

PROFILE_KEYS = {
    "schema_version",
    "profile_id",
    "fixture_id",
    "validated_parity_source_sha",
    "engine_sim_commit",
    "engine_sim_tree",
    "solver_commit",
    "fixture_semantic_contract_sha256",
    "ir_wav_sha256",
    "generated_ir_sha256",
    "simulation_frequency_hz",
    "sample_rate_hz",
    "deterministic_seed",
    "startup_throttle",
    "starter_disengagement_criterion",
    "post_starter_minimum_rpm",
    "stability_window_seconds",
    "maximum_startup_simulation_seconds",
    "candidate_sweep",
    "selected_candidate_id",
    "selected_trial_count",
    "success_count",
    "failure_count",
    "observed_distributions",
    "selected_margins",
    "selection_requirements",
    "positive_traces",
    "timeout_trace",
    "stall_trace",
    "generator",
    "verifier",
    "generated_header",
    "no_runtime_wav_dependency",
    "calibration_status",
    "payload_sha256",
}
TRACE_KEYS = {
    "schema_version",
    "fixture_id",
    "validated_parity_source_sha",
    "engine_sim_commit",
    "engine_sim_tree",
    "solver_commit",
    "fixture_semantic_contract_sha256",
    "ir_wav_sha256",
    "generated_ir_sha256",
    "seed",
    "candidate_id",
    "trial_index",
    "startup_throttle",
    "starter_disengagement_rpm",
    "post_starter_minimum_rpm",
    "stability_window_seconds",
    "maximum_startup_simulation_seconds",
    "starter_disengagement_time_seconds",
    "starter_disengagement_rpm_observed",
    "minimum_post_starter_rpm",
    "mean_post_starter_rpm",
    "produced_native_frames",
    "produced_pcm_frames",
    "pcm_non_finite_samples",
    "pcm_peak",
    "pcm_rms",
    "post_start_pcm_rms",
    "result",
    "failure_reason",
    "cleanup",
    "cycles",
}
EXPECTED = {
    "profile_id": "SubaruEJ25AtgVideo2ColdStartV1",
    "fixture_id": "SubaruEJ25AtgVideo2",
    "validated_parity_source_sha": "542d3261efc3ef48c78f337d990793aea55dd7fb",
    "engine_sim_commit": "85f7c3b959a908ed5232ede4f1a4ac7eafe6b630",
    "engine_sim_tree": "0756dea0444ada160540685dd1d28fcd3ef4aac5",
    "solver_commit": "e009f4ff1c9c4c5874e865e893cdb62e208fb2b3",
    "fixture_semantic_contract_sha256": "3ba447789024d613cdceb2382d917e9d6ce340cbeecb621d4f71133e55578f00",
    "ir_wav_sha256": "df0b8be829d28ae64e5b2818a1942a3b3e5733186bdf262cad4c2af038995d48",
    "generated_ir_sha256": "ce0702aa501d94f35b5f804efcd1b21688b9f9cacaa0fa2fc7f459c03a98e540",
}
GENERATOR_PATH = "Tools/EngineSimVendor/generate_cold_start_profile_header.py"
VERIFIER_PATH = "Tools/EngineSimVendor/verify_cold_start_profile.py"
HEADER_PATH = (
    "Plugins/NextcarEngineSim/Source/NextcarEngineSimCore/Private/Generated/"
    "SubaruEJ25ColdStartProfile.generated.h"
)
GENERATOR_COMMAND = (
    "python Tools/EngineSimVendor/generate_cold_start_profile_header.py "
    "--profile Tests/EngineSimCore/Fixtures/subaru_ej25_cold_start_profile.json "
    "--output Plugins/NextcarEngineSim/Source/NextcarEngineSimCore/Private/"
    "Generated/SubaruEJ25ColdStartProfile.generated.h"
)


class VerificationError(RuntimeError):
    pass


def reject_constant(value: str) -> None:
    raise VerificationError(f"non-finite JSON token: {value}")


def load_json(path: Path) -> Any:
    try:
        return json.loads(
            path.read_text(encoding="utf-8"),
            parse_constant=reject_constant,
        )
    except (OSError, json.JSONDecodeError) as exc:
        raise VerificationError(f"{path}: {exc}") from exc


def sha256_file(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require_exact_keys(value: dict[str, Any], expected: set[str], label: str) -> None:
    missing = sorted(expected - set(value))
    unknown = sorted(set(value) - expected)
    if missing or unknown:
        raise VerificationError(
            f"{label} keys mismatch; missing={missing}, unknown={unknown}"
        )


def require_finite(value: Any, label: str = "value") -> None:
    if value is None or isinstance(value, (str, bool, int)):
        return
    if isinstance(value, float):
        if not math.isfinite(value):
            raise VerificationError(f"{label} is not finite")
        return
    if isinstance(value, list):
        for index, item in enumerate(value):
            require_finite(item, f"{label}[{index}]")
        return
    if isinstance(value, dict):
        for key, item in value.items():
            require_finite(item, f"{label}.{key}")
        return
    raise VerificationError(f"{label} has unsupported type")


def validate_sha(value: Any, label: str) -> None:
    if not isinstance(value, str) or not re.fullmatch(r"[0-9a-f]{64}", value):
        raise VerificationError(f"{label} is not a lowercase SHA-256")


def canonical_payload_sha256(profile: dict[str, Any]) -> str:
    payload = deepcopy(profile)
    payload["payload_sha256"] = ""
    payload["generated_header"]["sha256"] = ""
    encoded = json.dumps(
        payload,
        ensure_ascii=False,
        sort_keys=True,
        separators=(",", ":"),
        allow_nan=False,
    ).encode("utf-8")
    return hashlib.sha256(encoded).hexdigest()


def percentile(values: list[float], probability: float) -> float:
    ordered = sorted(values)
    if len(ordered) == 1:
        return ordered[0]
    position = (len(ordered) - 1) * probability
    lower = math.floor(position)
    upper = math.ceil(position)
    if lower == upper:
        return ordered[lower]
    fraction = position - lower
    return ordered[lower] * (1.0 - fraction) + ordered[upper] * fraction


def distribution(values: list[float]) -> dict[str, float]:
    if not values:
        raise VerificationError("empty measured distribution")
    return {
        "minimum": min(values),
        "maximum": max(values),
        "mean": statistics.fmean(values),
        "median": statistics.median(values),
        "p95": percentile(values, 0.95),
    }


def close(actual: float, expected: float) -> bool:
    return math.isclose(actual, expected, rel_tol=1e-9, abs_tol=1e-9)


def compare_distribution(
    actual: dict[str, Any],
    expected: dict[str, float],
    label: str,
) -> None:
    require_exact_keys(
        actual,
        {"minimum", "maximum", "mean", "median", "p95"},
        label,
    )
    for key, expected_value in expected.items():
        if not close(float(actual[key]), float(expected_value)):
            raise VerificationError(f"{label}.{key} mismatch")


def validate_profile_static(profile: dict[str, Any]) -> None:
    require_exact_keys(profile, PROFILE_KEYS, "profile")
    require_finite(profile, "profile")
    if profile["schema_version"] != 1:
        raise VerificationError("unsupported profile schema")
    for key, expected in EXPECTED.items():
        if profile[key] != expected:
            raise VerificationError(f"profile pin mismatch: {key}")
    if profile["simulation_frequency_hz"] != 20000:
        raise VerificationError("simulation frequency mismatch")
    if profile["sample_rate_hz"] != 44100:
        raise VerificationError("sample rate mismatch")
    if profile["deterministic_seed"] != 0x4E433033:
        raise VerificationError("seed mismatch")
    if not 0.0 <= profile["startup_throttle"] <= 1.0:
        raise VerificationError("startup throttle out of range")
    criterion = profile["starter_disengagement_criterion"]
    if not isinstance(criterion, dict):
        raise VerificationError("criterion must be an object")
    require_exact_keys(criterion, {"type", "rpm"}, "criterion")
    if criterion["type"] != "rpm_at_or_above" or criterion["rpm"] <= 0:
        raise VerificationError("criterion mismatch")
    if profile["selected_trial_count"] < 10:
        raise VerificationError("fewer than 10 selected trials")
    if profile["success_count"] != profile["selected_trial_count"]:
        raise VerificationError("selected success count mismatch")
    if profile["failure_count"] != 0:
        raise VerificationError("selected failure count is non-zero")
    if len(profile["positive_traces"]) != profile["selected_trial_count"]:
        raise VerificationError("positive trace count mismatch")
    timeout_trace = profile["timeout_trace"]
    stall_trace = profile["stall_trace"]
    if not isinstance(timeout_trace, dict):
        raise VerificationError("timeout trace missing or invalid")
    if not isinstance(stall_trace, dict):
        raise VerificationError("stall trace missing or invalid")
    if timeout_trace.get("result") != "Timeout":
        raise VerificationError("timeout trace missing or invalid")
    if stall_trace.get("result") != "StallAfterDisengagement":
        raise VerificationError("stall trace missing or invalid")
    requirements = profile["selection_requirements"]
    margins = profile["selected_margins"]
    if margins["post_starter_rpm"] < requirements["minimum_post_starter_rpm_margin"]:
        raise VerificationError("post-starter RPM margin is insufficient")
    if margins["timeout_seconds"] < requirements["minimum_timeout_margin_seconds"]:
        raise VerificationError("timeout margin is insufficient")
    if (
        margins["disengagement_separation_rpm"]
        < requirements["minimum_disengagement_separation_rpm"]
    ):
        raise VerificationError("disengagement margin is insufficient")
    if profile["generator"]["path"] != GENERATOR_PATH:
        raise VerificationError("generator path mismatch")
    if profile["generator"]["command"] != GENERATOR_COMMAND:
        raise VerificationError("generator command mismatch")
    if profile["verifier"]["path"] != VERIFIER_PATH:
        raise VerificationError("verifier path mismatch")
    if profile["generated_header"]["path"] != HEADER_PATH:
        raise VerificationError("generated header path mismatch")
    for label, value in (
        ("payload", profile["payload_sha256"]),
        ("generator", profile["generator"]["sha256"]),
        ("verifier", profile["verifier"]["sha256"]),
        ("generated header", profile["generated_header"]["sha256"]),
        ("sweep", profile["candidate_sweep"]["sha256"]),
    ):
        validate_sha(value, label)
    if canonical_payload_sha256(profile) != profile["payload_sha256"]:
        raise VerificationError("payload SHA mismatch")
    if profile["no_runtime_wav_dependency"] is not True:
        raise VerificationError("runtime WAV dependency flag mismatch")


def validate_trace(
    trace: dict[str, Any],
    profile: dict[str, Any],
    expected_result: str,
    expected_trial: int | None,
) -> None:
    require_exact_keys(trace, TRACE_KEYS, "trace")
    require_finite(trace, "trace")
    for key in (
        "fixture_id",
        "validated_parity_source_sha",
        "engine_sim_commit",
        "engine_sim_tree",
        "solver_commit",
        "fixture_semantic_contract_sha256",
        "ir_wav_sha256",
        "generated_ir_sha256",
    ):
        if trace[key] != profile[key]:
            raise VerificationError(f"trace/profile mismatch: {key}")
    if trace["seed"] != profile["deterministic_seed"]:
        raise VerificationError("trace seed mismatch")
    if trace["result"] != expected_result:
        raise VerificationError(
            f"trace result {trace['result']} != {expected_result}"
        )
    if expected_trial is not None and trace["trial_index"] != expected_trial:
        raise VerificationError("trace trial index mismatch")
    cleanup = trace["cleanup"]
    if not isinstance(cleanup, dict) or not all(cleanup.values()):
        raise VerificationError("trace cleanup is incomplete")
    if trace["pcm_non_finite_samples"] != 0:
        raise VerificationError("trace contains non-finite PCM")
    if trace["produced_native_frames"] != sum(
        int(cycle["produced_native_frames"]) for cycle in trace["cycles"]
    ):
        raise VerificationError("native frame accounting mismatch")
    if trace["produced_pcm_frames"] != sum(
        int(cycle["produced_pcm_frames"]) for cycle in trace["cycles"]
    ):
        raise VerificationError("PCM frame accounting mismatch")
    if expected_result == "Success":
        if trace["failure_reason"] != "":
            raise VerificationError("successful trace has a failure reason")
        if trace["candidate_id"] != profile["selected_candidate_id"]:
            raise VerificationError("selected candidate ID mismatch")
        if not close(trace["startup_throttle"], profile["startup_throttle"]):
            raise VerificationError("startup throttle mismatch")
        if not close(
            trace["starter_disengagement_rpm"],
            profile["starter_disengagement_criterion"]["rpm"],
        ):
            raise VerificationError("disengagement criterion mismatch")
        if not close(
            trace["post_starter_minimum_rpm"],
            profile["post_starter_minimum_rpm"],
        ):
            raise VerificationError("minimum RPM mismatch")
        if not close(
            trace["stability_window_seconds"],
            profile["stability_window_seconds"],
        ):
            raise VerificationError("stability window mismatch")
        if not close(
            trace["maximum_startup_simulation_seconds"],
            profile["maximum_startup_simulation_seconds"],
        ):
            raise VerificationError("maximum time mismatch")
        if trace["produced_native_frames"] <= 0:
            raise VerificationError("successful trace has no native frames")
        if trace["produced_pcm_frames"] <= 0:
            raise VerificationError("successful trace has no PCM frames")
        if trace["post_start_pcm_rms"] <= 0.0:
            raise VerificationError("successful trace has zero post-start RMS")
        if trace["minimum_post_starter_rpm"] < profile["post_starter_minimum_rpm"]:
            raise VerificationError("successful trace violates minimum RPM")


def parse_header(path: Path) -> dict[str, str]:
    text = path.read_text(encoding="utf-8")
    if "\r" in text:
        raise VerificationError("generated header is not LF-only")
    if any(token in text.lower() for token in (".wav", "fopen", "ifstream")):
        raise VerificationError("generated header contains runtime WAV I/O")
    strings = dict(
        re.findall(
            r"inline constexpr char ([A-Za-z0-9_]+)\[\]\s*=\s*\n?\s*\"([^\"]*)\";",
            text,
        )
    )
    numbers = dict(
        re.findall(
            r"inline constexpr (?:std::uint32_t|int|double) "
            r"([A-Za-z0-9_]+)\s*=\s*\n?\s*([^;]+);",
            text,
        )
    )
    return {**strings, **numbers}


def verify_header_fields(header: dict[str, str], profile: dict[str, Any]) -> None:
    expected_strings = {
        "SubaruEJ25ColdStartProfileId": profile["profile_id"],
        "SubaruEJ25ColdStartProfileFixtureId": profile["fixture_id"],
        "SubaruEJ25ColdStartProfileValidatedParitySourceSha": profile[
            "validated_parity_source_sha"
        ],
        "SubaruEJ25ColdStartProfileEngineSimCommit": profile["engine_sim_commit"],
        "SubaruEJ25ColdStartProfileEngineSimTree": profile["engine_sim_tree"],
        "SubaruEJ25ColdStartProfileSolverCommit": profile["solver_commit"],
        "SubaruEJ25ColdStartProfileFixtureSemanticContractSha256": profile[
            "fixture_semantic_contract_sha256"
        ],
        "SubaruEJ25ColdStartProfileIrWavSha256": profile["ir_wav_sha256"],
        "SubaruEJ25ColdStartProfileGeneratedIrSha256": profile[
            "generated_ir_sha256"
        ],
        "SubaruEJ25ColdStartProfilePayloadSha256": profile["payload_sha256"],
        "SubaruEJ25ColdStartProfileSelectedCandidateId": profile[
            "selected_candidate_id"
        ],
    }
    for key, expected in expected_strings.items():
        if header.get(key) != str(expected):
            raise VerificationError(f"generated header string mismatch: {key}")

    expected_numbers = {
        "SubaruEJ25ColdStartProfileSeed": profile["deterministic_seed"],
        "SubaruEJ25ColdStartProfileSimulationFrequencyHz": profile[
            "simulation_frequency_hz"
        ],
        "SubaruEJ25ColdStartProfileSampleRateHz": profile["sample_rate_hz"],
        "SubaruEJ25ColdStartProfileStartupThrottle": profile["startup_throttle"],
        "SubaruEJ25ColdStartProfileStarterDisengagementRpm": profile[
            "starter_disengagement_criterion"
        ]["rpm"],
        "SubaruEJ25ColdStartProfilePostStarterMinimumRpm": profile[
            "post_starter_minimum_rpm"
        ],
        "SubaruEJ25ColdStartProfileStabilityWindowSeconds": profile[
            "stability_window_seconds"
        ],
        "SubaruEJ25ColdStartProfileMaximumStartupSimulationSeconds": profile[
            "maximum_startup_simulation_seconds"
        ],
        "SubaruEJ25ColdStartProfileTrialCount": profile["selected_trial_count"],
        "SubaruEJ25ColdStartProfileSuccessCount": profile["success_count"],
    }
    for key, expected in expected_numbers.items():
        raw = header.get(key)
        if raw is None:
            raise VerificationError(f"generated header number missing: {key}")
        raw = raw.rstrip("u")
        if not close(float(raw), float(expected)):
            raise VerificationError(f"generated header number mismatch: {key}")


def verify(
    profile_path: Path,
    repo_root: Path,
    *,
    run_generator: bool,
) -> dict[str, Any]:
    profile = load_json(profile_path)
    if not isinstance(profile, dict):
        raise VerificationError("profile root must be an object")
    validate_profile_static(profile)

    generator_path = repo_root / profile["generator"]["path"]
    verifier_path = repo_root / profile["verifier"]["path"]
    header_path = repo_root / profile["generated_header"]["path"]
    sweep_path = repo_root / profile["candidate_sweep"]["path"]
    for path, expected_hash, label in (
        (generator_path, profile["generator"]["sha256"], "generator"),
        (verifier_path, profile["verifier"]["sha256"], "verifier"),
        (header_path, profile["generated_header"]["sha256"], "generated header"),
        (sweep_path, profile["candidate_sweep"]["sha256"], "sweep"),
    ):
        if not path.is_file():
            raise VerificationError(f"{label} path does not exist: {path}")
        if sha256_file(path) != expected_hash:
            raise VerificationError(f"{label} SHA mismatch")

    positive: list[dict[str, Any]] = []
    for index, record in enumerate(profile["positive_traces"]):
        validate_sha(record["sha256"], f"positive trace {index}")
        path = repo_root / record["path"]
        if not path.is_file() or sha256_file(path) != record["sha256"]:
            raise VerificationError(f"positive trace {index} SHA/path mismatch")
        trace = load_json(path)
        if not isinstance(trace, dict):
            raise VerificationError("positive trace root must be an object")
        validate_trace(trace, profile, "Success", index)
        positive.append(trace)

    timeout_path = repo_root / profile["timeout_trace"]["path"]
    stall_path = repo_root / profile["stall_trace"]["path"]
    for path, record, expected_result in (
        (timeout_path, profile["timeout_trace"], "Timeout"),
        (stall_path, profile["stall_trace"], "StallAfterDisengagement"),
    ):
        validate_sha(record["sha256"], f"{expected_result} trace")
        if not path.is_file() or sha256_file(path) != record["sha256"]:
            raise VerificationError(f"{expected_result} trace SHA/path mismatch")
        trace = load_json(path)
        if not isinstance(trace, dict):
            raise VerificationError(f"{expected_result} trace root invalid")
        validate_trace(trace, profile, expected_result, None)
        if expected_result == "Timeout" and trace["starter_disengagement_time_seconds"] is not None:
            raise VerificationError("timeout case unexpectedly disengaged starter")
        if expected_result == "StallAfterDisengagement":
            if trace["starter_disengagement_time_seconds"] is None:
                raise VerificationError("stall case did not disengage starter")
            if not (
                trace["minimum_post_starter_rpm"]
                < trace["post_starter_minimum_rpm"]
            ):
                raise VerificationError("stall case did not fall below minimum")

    measured = {
        "disengagement_time_seconds": distribution(
            [float(trace["starter_disengagement_time_seconds"]) for trace in positive]
        ),
        "disengagement_rpm": distribution(
            [float(trace["starter_disengagement_rpm_observed"]) for trace in positive]
        ),
        "minimum_post_starter_rpm": distribution(
            [float(trace["minimum_post_starter_rpm"]) for trace in positive]
        ),
        "mean_post_starter_rpm": distribution(
            [float(trace["mean_post_starter_rpm"]) for trace in positive]
        ),
        "produced_native_frames": distribution(
            [float(trace["produced_native_frames"]) for trace in positive]
        ),
        "produced_pcm_frames": distribution(
            [float(trace["produced_pcm_frames"]) for trace in positive]
        ),
        "post_start_pcm_rms": distribution(
            [float(trace["post_start_pcm_rms"]) for trace in positive]
        ),
    }
    for key, expected in measured.items():
        compare_distribution(
            profile["observed_distributions"][key],
            expected,
            f"distribution {key}",
        )

    worst_completion = max(
        float(trace["starter_disengagement_time_seconds"])
        + float(profile["stability_window_seconds"])
        for trace in positive
    )
    expected_margins = {
        "post_starter_rpm": (
            measured["minimum_post_starter_rpm"]["minimum"]
            - float(profile["post_starter_minimum_rpm"])
        ),
        "timeout_seconds": (
            float(profile["maximum_startup_simulation_seconds"])
            - worst_completion
        ),
        "disengagement_separation_rpm": (
            measured["disengagement_rpm"]["minimum"]
            - float(profile["starter_disengagement_criterion"]["rpm"])
        ),
    }
    for key, expected in expected_margins.items():
        if not close(float(profile["selected_margins"][key]), expected):
            raise VerificationError(f"selected margin mismatch: {key}")

    header = parse_header(header_path)
    verify_header_fields(header, profile)

    if run_generator:
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "generated.h"
            command = [
                sys.executable,
                str(generator_path),
                "--profile",
                str(profile_path),
                "--output",
                str(output),
            ]
            completed = subprocess.run(
                command,
                check=False,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                cwd=repo_root,
            )
            if completed.returncode != 0:
                raise VerificationError(
                    "generator determinism command failed: "
                    + completed.stdout.strip()
                )
            if output.read_bytes() != header_path.read_bytes():
                raise VerificationError("generator output is not deterministic")

    return {
        "profile_json_sha256": sha256_file(profile_path),
        "payload_sha256": profile["payload_sha256"],
        "generated_header_sha256": sha256_file(header_path),
        "selected_trial_count": len(positive),
        "success_count": len(positive),
        "timeout_result": "Timeout",
        "stall_result": "StallAfterDisengagement",
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--profile", required=True, type=Path)
    parser.add_argument("--repo-root", default=Path("."), type=Path)
    parser.add_argument("--skip-generator-run", action="store_true")
    args = parser.parse_args(argv)
    try:
        result = verify(
            args.profile.resolve(),
            args.repo_root.resolve(),
            run_generator=not args.skip_generator_run,
        )
    except VerificationError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    print(json.dumps(result, sort_keys=True, separators=(",", ":")))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
