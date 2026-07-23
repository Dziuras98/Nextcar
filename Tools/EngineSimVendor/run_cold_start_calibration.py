#!/usr/bin/env python3
"""Run deterministic Subaru EJ25 cold-start discovery and selected-profile trials."""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import statistics
import subprocess
import sys
from copy import deepcopy
from pathlib import Path
from typing import Any, Iterable

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
CYCLE_KEYS = {
    "cycle_index",
    "simulation_time_seconds",
    "ignition_enabled",
    "starter_enabled",
    "current_rpm",
    "starter_disengagement_rpm_observed",
    "produced_native_frames",
    "produced_pcm_frames",
    "pcm_non_finite_samples",
    "pcm_peak",
    "pcm_rms",
}
CLEANUP_KEYS = {
    "starter_disabled",
    "ignition_disabled",
    "simulator_destroyed",
    "fixture_destroyed",
}
SWEEP_KEYS = {
    "schema_version",
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
    "simulation_slice_seconds",
    "startup_throttle",
    "starter_disengagement_rpm",
    "post_starter_minimum_rpm",
    "stability_window_seconds",
    "maximum_startup_simulation_seconds",
    "discovery_trace_count",
    "evaluated_candidate_count",
    "selection_requirements",
}
PIN_FIELDS = (
    "fixture_id",
    "validated_parity_source_sha",
    "engine_sim_commit",
    "engine_sim_tree",
    "solver_commit",
    "fixture_semantic_contract_sha256",
    "ir_wav_sha256",
    "generated_ir_sha256",
)
EXPECTED_PINS = {
    "fixture_id": "SubaruEJ25AtgVideo2",
    "validated_parity_source_sha": "542d3261efc3ef48c78f337d990793aea55dd7fb",
    "engine_sim_commit": "85f7c3b959a908ed5232ede4f1a4ac7eafe6b630",
    "engine_sim_tree": "0756dea0444ada160540685dd1d28fcd3ef4aac5",
    "solver_commit": "e009f4ff1c9c4c5874e865e893cdb62e208fb2b3",
    "fixture_semantic_contract_sha256": "3ba447789024d613cdceb2382d917e9d6ce340cbeecb621d4f71133e55578f00",
    "ir_wav_sha256": "df0b8be829d28ae64e5b2818a1942a3b3e5733186bdf262cad4c2af038995d48",
    "generated_ir_sha256": "ce0702aa501d94f35b5f804efcd1b21688b9f9cacaa0fa2fc7f459c03a98e540",
}
PROFILE_SCHEMA_VERSION = 1
PROFILE_ID = "SubaruEJ25AtgVideo2ColdStartV1"
PROFILE_PATH = "Tests/EngineSimCore/Fixtures/subaru_ej25_cold_start_profile.json"
HEADER_PATH = (
    "Plugins/NextcarEngineSim/Source/NextcarEngineSimCore/Private/Generated/"
    "SubaruEJ25ColdStartProfile.generated.h"
)
GENERATOR_PATH = "Tools/EngineSimVendor/generate_cold_start_profile_header.py"
VERIFIER_PATH = "Tools/EngineSimVendor/verify_cold_start_profile.py"
GENERATOR_COMMAND = (
    "python Tools/EngineSimVendor/generate_cold_start_profile_header.py "
    "--profile Tests/EngineSimCore/Fixtures/subaru_ej25_cold_start_profile.json "
    "--output Plugins/NextcarEngineSim/Source/NextcarEngineSimCore/Private/"
    "Generated/SubaruEJ25ColdStartProfile.generated.h"
)


class CalibrationError(RuntimeError):
    pass


def reject_constant(value: str) -> None:
    raise CalibrationError(f"non-finite JSON token: {value}")


def load_json(path: Path) -> Any:
    try:
        return json.loads(
            path.read_text(encoding="utf-8"),
            parse_constant=reject_constant,
        )
    except (OSError, json.JSONDecodeError) as exc:
        raise CalibrationError(f"{path}: {exc}") from exc


def write_json(path: Path, value: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    text = json.dumps(
        value,
        ensure_ascii=False,
        sort_keys=True,
        indent=2,
        allow_nan=False,
    ) + "\n"
    path.write_text(text, encoding="utf-8", newline="\n")


def sha256_file(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require_exact_keys(value: dict[str, Any], expected: set[str], label: str) -> None:
    actual = set(value)
    missing = sorted(expected - actual)
    unknown = sorted(actual - expected)
    if missing or unknown:
        raise CalibrationError(
            f"{label} keys mismatch; missing={missing}, unknown={unknown}"
        )


def require_finite(value: Any, label: str = "value") -> None:
    if isinstance(value, bool) or value is None or isinstance(value, str):
        return
    if isinstance(value, int):
        return
    if isinstance(value, float):
        if not math.isfinite(value):
            raise CalibrationError(f"{label} is not finite")
        return
    if isinstance(value, list):
        for index, item in enumerate(value):
            require_finite(item, f"{label}[{index}]")
        return
    if isinstance(value, dict):
        for key, item in value.items():
            require_finite(item, f"{label}.{key}")
        return
    raise CalibrationError(f"{label} has unsupported type {type(value).__name__}")


def validate_sweep(sweep: dict[str, Any]) -> None:
    require_exact_keys(sweep, SWEEP_KEYS, "sweep")
    require_finite(sweep, "sweep")
    if sweep["schema_version"] != 1:
        raise CalibrationError("unsupported sweep schema")
    for key, expected in EXPECTED_PINS.items():
        if sweep[key] != expected:
            raise CalibrationError(f"sweep pin mismatch: {key}")
    if sweep["simulation_frequency_hz"] != 20000:
        raise CalibrationError("simulation frequency mismatch")
    if sweep["sample_rate_hz"] != 44100:
        raise CalibrationError("sample rate mismatch")
    if sweep["deterministic_seed"] != 0x4E433033:
        raise CalibrationError("deterministic seed mismatch")
    if not math.isclose(sweep["simulation_slice_seconds"], 1.0 / 120.0):
        raise CalibrationError("simulation slice mismatch")

    dimensions = (
        sweep["startup_throttle"],
        sweep["starter_disengagement_rpm"],
        sweep["post_starter_minimum_rpm"],
        sweep["stability_window_seconds"],
        sweep["maximum_startup_simulation_seconds"],
    )
    if any(not isinstance(values, list) or not values for values in dimensions):
        raise CalibrationError("sweep dimensions must be non-empty lists")
    candidate_count = math.prod(len(values) for values in dimensions)
    trace_count = (
        len(sweep["startup_throttle"])
        * len(sweep["starter_disengagement_rpm"])
    )
    if candidate_count != 1080 or sweep["evaluated_candidate_count"] != 1080:
        raise CalibrationError("sweep must evaluate exactly 1080 candidates")
    if trace_count != 30 or sweep["discovery_trace_count"] != 30:
        raise CalibrationError("sweep must produce exactly 30 discovery traces")


def validate_trace(trace: dict[str, Any], sweep: dict[str, Any] | None = None) -> None:
    require_exact_keys(trace, TRACE_KEYS, "trace")
    require_finite(trace, "trace")
    if trace["schema_version"] != 1:
        raise CalibrationError("unsupported trace schema")
    for key, expected in EXPECTED_PINS.items():
        if trace[key] != expected:
            raise CalibrationError(f"trace pin mismatch: {key}")
    if not isinstance(trace["cycles"], list) or not trace["cycles"]:
        raise CalibrationError("trace cycles must be non-empty")
    for index, cycle in enumerate(trace["cycles"]):
        if not isinstance(cycle, dict):
            raise CalibrationError(f"cycle {index} is not an object")
        require_exact_keys(cycle, CYCLE_KEYS, f"cycle {index}")
        require_finite(cycle, f"cycle {index}")
        if cycle["cycle_index"] != index:
            raise CalibrationError("cycle indices are not contiguous")
        if cycle["produced_native_frames"] < 0 or cycle["produced_pcm_frames"] < 0:
            raise CalibrationError("negative produced frame count")
        if cycle["pcm_non_finite_samples"] != 0:
            raise CalibrationError("trace contains non-finite PCM")
    cleanup = trace["cleanup"]
    if not isinstance(cleanup, dict):
        raise CalibrationError("cleanup must be an object")
    require_exact_keys(cleanup, CLEANUP_KEYS, "cleanup")
    if not all(cleanup.values()):
        raise CalibrationError("trace cleanup is incomplete")
    if trace["produced_native_frames"] != sum(
        cycle["produced_native_frames"] for cycle in trace["cycles"]
    ):
        raise CalibrationError("native frame accounting mismatch")
    if trace["produced_pcm_frames"] != sum(
        cycle["produced_pcm_frames"] for cycle in trace["cycles"]
    ):
        raise CalibrationError("PCM frame accounting mismatch")
    if trace["pcm_non_finite_samples"] != 0:
        raise CalibrationError("top-level non-finite PCM count")
    if sweep is not None:
        for key in PIN_FIELDS:
            if trace[key] != sweep[key]:
                raise CalibrationError(f"trace/sweep mismatch: {key}")
        if trace["seed"] != sweep["deterministic_seed"]:
            raise CalibrationError("trace seed mismatch")


def run_trial(
    executable: Path,
    output: Path,
    *,
    candidate_id: str,
    trial_index: int,
    seed: int,
    throttle: float,
    disengagement_rpm: float,
    post_min_rpm: float,
    stability_window: float,
    maximum_time: float,
    disable_ignition_after_disengagement: bool = False,
) -> dict[str, Any]:
    command = [
        str(executable),
        "--output",
        str(output),
        "--candidate-id",
        candidate_id,
        "--trial-index",
        str(trial_index),
        "--seed",
        hex(seed),
        "--startup-throttle",
        format(throttle, ".17g"),
        "--disengagement-rpm",
        format(disengagement_rpm, ".17g"),
        "--post-min-rpm",
        format(post_min_rpm, ".17g"),
        "--stability-window",
        format(stability_window, ".17g"),
        "--maximum-time",
        format(maximum_time, ".17g"),
    ]
    if disable_ignition_after_disengagement:
        command.append("--disable-ignition-after-disengagement")
    completed = subprocess.run(
        command,
        check=False,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if completed.returncode != 0:
        raise CalibrationError(
            f"trial command failed ({completed.returncode}): "
            + completed.stdout.strip()
        )
    trace = load_json(output)
    if not isinstance(trace, dict):
        raise CalibrationError("trace root must be an object")
    validate_trace(trace)
    return trace


def float_token(value: float) -> str:
    return format(value, ".3f").replace(".", "p")


def discovery_id(throttle: float, disengagement_rpm: float) -> str:
    return f"discovery-thr-{float_token(throttle)}-dis-{int(disengagement_rpm):04d}"


def candidate_id(
    throttle: float,
    disengagement_rpm: float,
    post_min_rpm: float,
    stability_window: float,
    maximum_time: float,
) -> str:
    return (
        f"thr-{float_token(throttle)}"
        f"-dis-{int(disengagement_rpm):04d}"
        f"-min-{int(post_min_rpm):04d}"
        f"-win-{float_token(stability_window)}"
        f"-max-{float_token(maximum_time)}"
    )


def weighted_rms(cycles: Iterable[dict[str, Any]]) -> float:
    square_sum = 0.0
    samples = 0
    for cycle in cycles:
        count = int(cycle["produced_pcm_frames"])
        square_sum += float(cycle["pcm_rms"]) ** 2 * count
        samples += count
    return math.sqrt(square_sum / samples) if samples else 0.0


def evaluate_candidate(
    trace: dict[str, Any],
    *,
    post_min_rpm: float,
    stability_window: float,
    maximum_time: float,
    requirements: dict[str, Any],
) -> dict[str, Any]:
    disengagement_time = trace["starter_disengagement_time_seconds"]
    observed_disengagement_rpm = trace["starter_disengagement_rpm_observed"]
    result: dict[str, Any] = {
        "success": False,
        "failure_reason": "",
        "disengagement_time_seconds": disengagement_time,
        "disengagement_rpm_observed": observed_disengagement_rpm,
        "minimum_post_starter_rpm": None,
        "mean_post_starter_rpm": None,
        "produced_native_frames": 0,
        "produced_pcm_frames": 0,
        "pcm_rms": 0.0,
        "post_starter_rpm_margin": None,
        "timeout_margin_seconds": None,
        "disengagement_separation_rpm": None,
    }
    if disengagement_time is None or observed_disengagement_rpm is None:
        result["failure_reason"] = "Timeout"
        return result

    completion_time = float(disengagement_time) + float(stability_window)
    result["timeout_margin_seconds"] = float(maximum_time) - completion_time
    result["disengagement_separation_rpm"] = (
        float(observed_disengagement_rpm)
        - float(trace["starter_disengagement_rpm"])
    )
    if completion_time > float(maximum_time) + 1e-12:
        result["failure_reason"] = "Timeout"
        return result

    cycles = [
        cycle
        for cycle in trace["cycles"]
        if not bool(cycle["starter_enabled"])
        and float(cycle["simulation_time_seconds"]) > float(disengagement_time)
        and float(cycle["simulation_time_seconds"]) - 1e-12
        <= completion_time
    ]
    if not cycles or float(cycles[-1]["simulation_time_seconds"]) + 1e-12 < completion_time:
        result["failure_reason"] = "TraceTooShort"
        return result

    rpms = [float(cycle["current_rpm"]) for cycle in cycles]
    minimum_rpm = min(rpms)
    result["minimum_post_starter_rpm"] = minimum_rpm
    result["mean_post_starter_rpm"] = statistics.fmean(rpms)
    result["produced_native_frames"] = sum(
        int(cycle["produced_native_frames"]) for cycle in cycles
    )
    result["produced_pcm_frames"] = sum(
        int(cycle["produced_pcm_frames"]) for cycle in cycles
    )
    result["pcm_rms"] = weighted_rms(cycles)
    result["post_starter_rpm_margin"] = minimum_rpm - post_min_rpm

    if minimum_rpm < post_min_rpm:
        result["failure_reason"] = "StallAfterDisengagement"
    elif result["produced_native_frames"] <= 0:
        result["failure_reason"] = "ZeroProducedNativeFrames"
    elif result["produced_pcm_frames"] <= 0:
        result["failure_reason"] = "ZeroProducedPcm"
    elif result["pcm_rms"] <= 0.0:
        result["failure_reason"] = "ZeroPostStartRms"
    elif result["post_starter_rpm_margin"] < float(
        requirements["minimum_post_starter_rpm_margin"]
    ):
        result["failure_reason"] = "InsufficientPostStarterRpmMargin"
    elif result["timeout_margin_seconds"] < float(
        requirements["minimum_timeout_margin_seconds"]
    ):
        result["failure_reason"] = "InsufficientTimeoutMargin"
    elif result["disengagement_separation_rpm"] < float(
        requirements["minimum_disengagement_separation_rpm"]
    ):
        result["failure_reason"] = "InsufficientDisengagementSeparation"
    else:
        result["success"] = True
    return result


def percentile(values: list[float], probability: float) -> float:
    ordered = sorted(values)
    if not ordered:
        raise CalibrationError("cannot compute percentile of empty values")
    if len(ordered) == 1:
        return ordered[0]
    position = (len(ordered) - 1) * probability
    lower = math.floor(position)
    upper = math.ceil(position)
    if lower == upper:
        return ordered[lower]
    fraction = position - lower
    return ordered[lower] * (1.0 - fraction) + ordered[upper] * fraction


def canonical_statistic(value: float) -> float:
    if not math.isfinite(value):
        raise CalibrationError("non-finite distribution statistic")
    return float(format(value, ".12f"))


def clamp_roundoff_to_bounds(
    value: float,
    minimum: float,
    maximum: float,
) -> float:
    scale = max(abs(value), abs(minimum), abs(maximum), 1.0)
    tolerance = max(1.0e-12, 8.0 * math.ulp(scale))

    if value < minimum:
        if minimum - value <= tolerance:
            return minimum
        raise CalibrationError("distribution statistic below minimum")

    if value > maximum:
        if value - maximum <= tolerance:
            return maximum
        raise CalibrationError("distribution statistic above maximum")

    return value


def distribution(values: list[float]) -> dict[str, float]:
    if not values:
        raise CalibrationError("empty distribution")

    canonical_values = [canonical_statistic(float(value)) for value in values]
    minimum = canonical_statistic(min(canonical_values))
    maximum = canonical_statistic(max(canonical_values))

    def bounded(value: float) -> float:
        return clamp_roundoff_to_bounds(
            canonical_statistic(value),
            minimum,
            maximum,
        )

    return {
        "minimum": minimum,
        "maximum": maximum,
        "mean": bounded(statistics.fmean(canonical_values)),
        "median": bounded(statistics.median(canonical_values)),
        "p95": bounded(percentile(canonical_values, 0.95)),
    }


def selection_sort_key(candidate: dict[str, Any]) -> tuple[Any, ...]:
    return (
        -float(candidate["stability_window_seconds"]),
        -float(candidate["post_starter_minimum_rpm"]),
        float(candidate["startup_throttle"]),
        float(candidate["maximum_startup_simulation_seconds"]),
        -float(candidate["starter_disengagement_rpm"]),
        candidate["candidate_id"],
    )


def discovery(args: argparse.Namespace) -> int:
    sweep_path = Path(args.sweep)
    sweep = load_json(sweep_path)
    if not isinstance(sweep, dict):
        raise CalibrationError("sweep root must be an object")
    validate_sweep(sweep)

    executable = Path(args.executable)
    output_root = Path(args.output)
    traces_dir = output_root / "discovery-traces"
    traces_dir.mkdir(parents=True, exist_ok=True)

    trace_by_pair: dict[tuple[float, float], dict[str, Any]] = {}
    trace_records: list[dict[str, Any]] = []
    longest_window = max(sweep["stability_window_seconds"])
    longest_time = max(sweep["maximum_startup_simulation_seconds"])

    for throttle in sweep["startup_throttle"]:
        for disengagement in sweep["starter_disengagement_rpm"]:
            identifier = discovery_id(float(throttle), float(disengagement))
            trace_path = traces_dir / f"{identifier}.json"
            trace = run_trial(
                executable,
                trace_path,
                candidate_id=identifier,
                trial_index=0,
                seed=int(sweep["deterministic_seed"]),
                throttle=float(throttle),
                disengagement_rpm=float(disengagement),
                post_min_rpm=0.0,
                stability_window=float(longest_window),
                maximum_time=float(longest_time),
            )
            validate_trace(trace, sweep)
            trace_by_pair[(float(throttle), float(disengagement))] = trace
            trace_records.append(
                {
                    "path": trace_path.relative_to(output_root).as_posix(),
                    "sha256": sha256_file(trace_path),
                    "result": trace["result"],
                    "candidate_id": identifier,
                }
            )

    candidates: list[dict[str, Any]] = []
    requirements = sweep["selection_requirements"]
    for throttle in sweep["startup_throttle"]:
        for disengagement in sweep["starter_disengagement_rpm"]:
            trace = trace_by_pair[(float(throttle), float(disengagement))]
            for post_min in sweep["post_starter_minimum_rpm"]:
                for window in sweep["stability_window_seconds"]:
                    for maximum_time in sweep["maximum_startup_simulation_seconds"]:
                        identifier = candidate_id(
                            float(throttle),
                            float(disengagement),
                            float(post_min),
                            float(window),
                            float(maximum_time),
                        )
                        evaluation = evaluate_candidate(
                            trace,
                            post_min_rpm=float(post_min),
                            stability_window=float(window),
                            maximum_time=float(maximum_time),
                            requirements=requirements,
                        )
                        candidates.append(
                            {
                                "candidate_id": identifier,
                                "startup_throttle": float(throttle),
                                "starter_disengagement_rpm": float(disengagement),
                                "post_starter_minimum_rpm": float(post_min),
                                "stability_window_seconds": float(window),
                                "maximum_startup_simulation_seconds": float(maximum_time),
                                "discovery_trace_path": next(
                                    record["path"]
                                    for record in trace_records
                                    if record["candidate_id"]
                                    == discovery_id(
                                        float(throttle),
                                        float(disengagement),
                                    )
                                ),
                                **evaluation,
                            }
                        )

    if len(candidates) != 1080:
        raise CalibrationError(
            f"expected 1080 evaluated candidates, got {len(candidates)}"
        )
    passing = sorted(
        (candidate for candidate in candidates if candidate["success"]),
        key=selection_sort_key,
    )
    selected = deepcopy(passing[0]) if passing else None

    summary = {
        "schema_version": 1,
        "candidate_source_sha": args.candidate_sha,
        "sweep_path": sweep_path.as_posix(),
        "sweep_sha256": sha256_file(sweep_path),
        "discovery_trace_count": len(trace_records),
        "evaluated_candidate_count": len(candidates),
        "passing_candidate_count": len(passing),
        "selection_policy": [
            "longest stability window",
            "highest measured-safe minimum RPM",
            "lowest startup throttle",
            "tightest maximum time with required margin",
            "highest disengagement criterion",
            "candidate ID lexical tie-break",
        ],
        "selected_candidate": selected,
        "traces": trace_records,
        "candidates": candidates,
    }
    write_json(output_root / "candidate-summary.json", summary)
    write_json(
        output_root / "selected-candidate.json",
        {
            "schema_version": 1,
            "candidate_source_sha": args.candidate_sha,
            "selected_candidate": selected,
        },
    )

    print(
        f"DISCOVERY traces={len(trace_records)} "
        f"candidates={len(candidates)} passing={len(passing)}"
    )
    if selected is None:
        print("DISCOVERY no accepted candidate")
        return 2
    print(f"DISCOVERY selected={selected['candidate_id']}")
    return 0


def metric_distributions(traces: list[dict[str, Any]]) -> dict[str, Any]:
    return {
        "disengagement_time_seconds": distribution(
            [float(trace["starter_disengagement_time_seconds"]) for trace in traces]
        ),
        "disengagement_rpm": distribution(
            [float(trace["starter_disengagement_rpm_observed"]) for trace in traces]
        ),
        "minimum_post_starter_rpm": distribution(
            [float(trace["minimum_post_starter_rpm"]) for trace in traces]
        ),
        "mean_post_starter_rpm": distribution(
            [float(trace["mean_post_starter_rpm"]) for trace in traces]
        ),
        "produced_native_frames": distribution(
            [float(trace["produced_native_frames"]) for trace in traces]
        ),
        "produced_pcm_frames": distribution(
            [float(trace["produced_pcm_frames"]) for trace in traces]
        ),
        "post_start_pcm_rms": distribution(
            [float(trace["post_start_pcm_rms"]) for trace in traces]
        ),
    }


def canonical_payload_hash(profile: dict[str, Any]) -> str:
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


def selected(args: argparse.Namespace) -> int:
    selected_document = load_json(Path(args.selected_candidate))
    if not isinstance(selected_document, dict):
        raise CalibrationError("selected candidate document must be an object")
    if set(selected_document) != {
        "schema_version",
        "candidate_source_sha",
        "selected_candidate",
    }:
        raise CalibrationError("selected candidate document keys mismatch")
    selected_candidate = selected_document["selected_candidate"]
    if not isinstance(selected_candidate, dict):
        raise CalibrationError("selected candidate is missing")

    sweep_path = Path(args.sweep)
    sweep = load_json(sweep_path)
    if not isinstance(sweep, dict):
        raise CalibrationError("sweep root must be an object")
    validate_sweep(sweep)

    executable = Path(args.executable)
    output_root = Path(args.output)
    repo_root = Path(args.repo_root).resolve()
    output_root = output_root.resolve()
    try:
        output_root.relative_to(repo_root)
    except ValueError as exc:
        raise CalibrationError("selected output must be inside repo root") from exc
    positive_dir = output_root / "positive-traces"
    positive_dir.mkdir(parents=True, exist_ok=True)
    positive_traces: list[dict[str, Any]] = []
    positive_records: list[dict[str, Any]] = []

    trial_count = int(args.trials)
    if trial_count < 10:
        raise CalibrationError("selected profile requires at least 10 trials")

    for trial_index in range(trial_count):
        trace_path = positive_dir / f"trial-{trial_index:02d}.json"
        trace = run_trial(
            executable,
            trace_path,
            candidate_id=selected_candidate["candidate_id"],
            trial_index=trial_index,
            seed=int(sweep["deterministic_seed"]),
            throttle=float(selected_candidate["startup_throttle"]),
            disengagement_rpm=float(
                selected_candidate["starter_disengagement_rpm"]
            ),
            post_min_rpm=float(
                selected_candidate["post_starter_minimum_rpm"]
            ),
            stability_window=float(
                selected_candidate["stability_window_seconds"]
            ),
            maximum_time=float(
                selected_candidate["maximum_startup_simulation_seconds"]
            ),
        )
        validate_trace(trace, sweep)
        if trace["result"] != "Success":
            raise CalibrationError(
                f"selected trial {trial_index} failed: {trace['result']}"
            )
        if trace["produced_native_frames"] <= 0:
            raise CalibrationError("selected trial produced no native frames")
        if trace["produced_pcm_frames"] <= 0:
            raise CalibrationError("selected trial produced no PCM frames")
        if trace["pcm_non_finite_samples"] != 0:
            raise CalibrationError("selected trial contains non-finite PCM")
        if trace["post_start_pcm_rms"] <= 0.0:
            raise CalibrationError("selected trial has zero post-start RMS")
        positive_traces.append(trace)
        positive_records.append(
            {
                "trial_index": trial_index,
                "path": trace_path.relative_to(repo_root).as_posix(),
                "sha256": sha256_file(trace_path),
                "result": trace["result"],
            }
        )

    timeout_path = output_root / "negative-timeout.json"
    timeout_trace = run_trial(
        executable,
        timeout_path,
        candidate_id="negative-timeout",
        trial_index=0,
        seed=int(sweep["deterministic_seed"]),
        throttle=float(selected_candidate["startup_throttle"]),
        disengagement_rpm=100000.0,
        post_min_rpm=float(selected_candidate["post_starter_minimum_rpm"]),
        stability_window=float(selected_candidate["stability_window_seconds"]),
        maximum_time=0.25,
    )
    if timeout_trace["result"] != "Timeout":
        raise CalibrationError(
            f"timeout negative case returned {timeout_trace['result']}"
        )

    stall_path = output_root / "negative-stall.json"
    stall_trace = run_trial(
        executable,
        stall_path,
        candidate_id="negative-stall-after-disengagement",
        trial_index=0,
        seed=int(sweep["deterministic_seed"]),
        throttle=float(selected_candidate["startup_throttle"]),
        disengagement_rpm=float(
            selected_candidate["starter_disengagement_rpm"]
        ),
        post_min_rpm=float(selected_candidate["post_starter_minimum_rpm"]),
        stability_window=float(selected_candidate["stability_window_seconds"]),
        maximum_time=max(
            10.0,
            float(selected_candidate["maximum_startup_simulation_seconds"]),
        ),
        disable_ignition_after_disengagement=True,
    )
    if stall_trace["result"] != "StallAfterDisengagement":
        raise CalibrationError(
            f"stall negative case returned {stall_trace['result']}"
        )
    if stall_trace["starter_disengagement_time_seconds"] is None:
        raise CalibrationError("stall case did not disengage starter")

    distributions = metric_distributions(positive_traces)
    worst_minimum_rpm = distributions["minimum_post_starter_rpm"]["minimum"]
    worst_completion_time = max(
        float(trace["starter_disengagement_time_seconds"])
        + float(selected_candidate["stability_window_seconds"])
        for trace in positive_traces
    )
    minimum_observed_disengagement = distributions["disengagement_rpm"]["minimum"]
    margins = {
        "post_starter_rpm": (
            worst_minimum_rpm
            - float(selected_candidate["post_starter_minimum_rpm"])
        ),
        "timeout_seconds": (
            float(selected_candidate["maximum_startup_simulation_seconds"])
            - worst_completion_time
        ),
        "disengagement_separation_rpm": (
            minimum_observed_disengagement
            - float(selected_candidate["starter_disengagement_rpm"])
        ),
    }
    requirements = sweep["selection_requirements"]
    if margins["post_starter_rpm"] < requirements[
        "minimum_post_starter_rpm_margin"
    ]:
        raise CalibrationError("selected RPM margin is insufficient")
    if margins["timeout_seconds"] < requirements[
        "minimum_timeout_margin_seconds"
    ]:
        raise CalibrationError("selected timeout margin is insufficient")
    if margins["disengagement_separation_rpm"] < requirements[
        "minimum_disengagement_separation_rpm"
    ]:
        raise CalibrationError("selected disengagement separation is insufficient")

    generator_path = repo_root / GENERATOR_PATH
    verifier_path = repo_root / VERIFIER_PATH
    if not generator_path.is_file() or not verifier_path.is_file():
        raise CalibrationError("generator or verifier path is missing")

    profile = {
        "schema_version": PROFILE_SCHEMA_VERSION,
        "profile_id": PROFILE_ID,
        **EXPECTED_PINS,
        "simulation_frequency_hz": 20000,
        "sample_rate_hz": 44100,
        "deterministic_seed": int(sweep["deterministic_seed"]),
        "startup_throttle": float(selected_candidate["startup_throttle"]),
        "starter_disengagement_criterion": {
            "type": "rpm_at_or_above",
            "rpm": float(selected_candidate["starter_disengagement_rpm"]),
        },
        "post_starter_minimum_rpm": float(
            selected_candidate["post_starter_minimum_rpm"]
        ),
        "stability_window_seconds": float(
            selected_candidate["stability_window_seconds"]
        ),
        "maximum_startup_simulation_seconds": float(
            selected_candidate["maximum_startup_simulation_seconds"]
        ),
        "candidate_sweep": {
            "path": "Tests/EngineSimCore/Fixtures/subaru_ej25_cold_start_sweep.json",
            "sha256": sha256_file(sweep_path),
            "evaluated_candidate_count": 1080,
            "startup_throttle": sweep["startup_throttle"],
            "starter_disengagement_rpm": sweep["starter_disengagement_rpm"],
            "post_starter_minimum_rpm": sweep["post_starter_minimum_rpm"],
            "stability_window_seconds": sweep["stability_window_seconds"],
            "maximum_startup_simulation_seconds": sweep[
                "maximum_startup_simulation_seconds"
            ],
        },
        "selected_candidate_id": selected_candidate["candidate_id"],
        "selected_trial_count": trial_count,
        "success_count": trial_count,
        "failure_count": 0,
        "observed_distributions": distributions,
        "selected_margins": margins,
        "selection_requirements": requirements,
        "positive_traces": positive_records,
        "timeout_trace": {
            "path": timeout_path.relative_to(repo_root).as_posix(),
            "sha256": sha256_file(timeout_path),
            "result": timeout_trace["result"],
        },
        "stall_trace": {
            "path": stall_path.relative_to(repo_root).as_posix(),
            "sha256": sha256_file(stall_path),
            "result": stall_trace["result"],
        },
        "generator": {
            "path": GENERATOR_PATH,
            "sha256": sha256_file(generator_path),
            "command": GENERATOR_COMMAND,
        },
        "verifier": {
            "path": VERIFIER_PATH,
            "sha256": sha256_file(verifier_path),
        },
        "generated_header": {
            "path": HEADER_PATH,
            "sha256": "",
        },
        "no_runtime_wav_dependency": True,
        "calibration_status": "windows_pass_candidate",
        "payload_sha256": "",
    }
    profile["payload_sha256"] = canonical_payload_hash(profile)
    profile_path = output_root / "profile-candidate.json"
    write_json(profile_path, profile)

    summary = {
        "schema_version": 1,
        "candidate_source_sha": args.candidate_sha,
        "selected_candidate": selected_candidate,
        "selected_trial_count": trial_count,
        "success_count": trial_count,
        "failure_count": 0,
        "observed_distributions": distributions,
        "selected_margins": margins,
        "positive_traces": positive_records,
        "timeout_trace": profile["timeout_trace"],
        "stall_trace": profile["stall_trace"],
        "profile_candidate_path": profile_path.relative_to(output_root).as_posix(),
        "profile_candidate_sha256": sha256_file(profile_path),
    }
    write_json(output_root / "selected-summary.json", summary)
    print(
        f"SELECTED candidate={selected_candidate['candidate_id']} "
        f"trials={trial_count} success={trial_count}"
    )
    return 0


def self_test() -> int:
    sample = {
        "cycles": [
            {
                "produced_pcm_frames": 2,
                "pcm_rms": 0.5,
            },
            {
                "produced_pcm_frames": 6,
                "pcm_rms": 0.25,
            },
        ]
    }
    value = weighted_rms(sample["cycles"])
    expected = math.sqrt((0.5 * 0.5 * 2 + 0.25 * 0.25 * 6) / 8)
    if not math.isclose(value, expected):
        raise CalibrationError("weighted RMS self-test failed")
    if percentile([1.0, 2.0, 3.0, 4.0], 0.95) <= 3.0:
        raise CalibrationError("percentile self-test failed")

    identical = distribution([1.941666667] * 10)
    expected_identical = {
        "minimum": 1.941666667,
        "maximum": 1.941666667,
        "mean": 1.941666667,
        "median": 1.941666667,
        "p95": 1.941666667,
    }
    if identical != expected_identical:
        raise CalibrationError("identical-value distribution canonicalization failed")

    try:
        clamp_roundoff_to_bounds(1.000001, 1.0, 1.0)
    except CalibrationError:
        pass
    else:
        raise CalibrationError("real distribution bound violation was masked")

    variable = distribution([1.0, 2.0, 3.0, 4.0])
    expected_variable = {
        "minimum": 1.0,
        "maximum": 4.0,
        "mean": 2.5,
        "median": 2.5,
        "p95": 3.85,
    }
    if variable != expected_variable:
        raise CalibrationError("variable distribution changed unexpectedly")

    print("SELF-TEST PASS")
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)

    discovery_parser = subparsers.add_parser("discovery")
    discovery_parser.add_argument("--executable", required=True)
    discovery_parser.add_argument("--sweep", required=True)
    discovery_parser.add_argument("--output", required=True)
    discovery_parser.add_argument("--candidate-sha", required=True)
    discovery_parser.set_defaults(function=discovery)

    selected_parser = subparsers.add_parser("selected")
    selected_parser.add_argument("--executable", required=True)
    selected_parser.add_argument("--sweep", required=True)
    selected_parser.add_argument("--selected-candidate", required=True)
    selected_parser.add_argument("--output", required=True)
    selected_parser.add_argument("--repo-root", required=True)
    selected_parser.add_argument("--candidate-sha", required=True)
    selected_parser.add_argument("--trials", type=int, default=10)
    selected_parser.set_defaults(function=selected)

    self_parser = subparsers.add_parser("self-test")
    self_parser.set_defaults(function=lambda _args: self_test())
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    try:
        return int(args.function(args))
    except CalibrationError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
