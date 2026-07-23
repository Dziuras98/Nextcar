#!/usr/bin/env python3
"""Generate the deterministic Subaru EJ25 cold-start profile C++ header."""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
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
EXPECTED_PINS = {
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


class ProfileError(ValueError):
    pass


def reject_constant(value: str) -> None:
    raise ProfileError(f"non-finite JSON token: {value}")


def load_profile(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(
            path.read_text(encoding="utf-8"),
            parse_constant=reject_constant,
        )
    except (OSError, json.JSONDecodeError) as exc:
        raise ProfileError(str(exc)) from exc
    if not isinstance(value, dict):
        raise ProfileError("profile root must be an object")
    return value


def require_exact_keys(value: dict[str, Any], expected: set[str], label: str) -> None:
    missing = sorted(expected - set(value))
    unknown = sorted(set(value) - expected)
    if missing or unknown:
        raise ProfileError(
            f"{label} keys mismatch; missing={missing}, unknown={unknown}"
        )


def require_finite(value: Any, label: str = "value") -> None:
    if value is None or isinstance(value, (str, bool, int)):
        return
    if isinstance(value, float):
        if not math.isfinite(value):
            raise ProfileError(f"{label} is not finite")
        return
    if isinstance(value, list):
        for index, item in enumerate(value):
            require_finite(item, f"{label}[{index}]")
        return
    if isinstance(value, dict):
        for key, item in value.items():
            require_finite(item, f"{label}.{key}")
        return
    raise ProfileError(f"{label} has unsupported type")


def canonical_payload_sha256(profile: dict[str, Any]) -> str:
    payload = deepcopy(profile)
    payload["payload_sha256"] = ""
    payload["generated_header"]["sha256"] = ""
    data = json.dumps(
        payload,
        ensure_ascii=False,
        sort_keys=True,
        separators=(",", ":"),
        allow_nan=False,
    ).encode("utf-8")
    return hashlib.sha256(data).hexdigest()


def validate_sha(value: Any, label: str, allow_empty: bool = False) -> None:
    if allow_empty and value == "":
        return
    if not isinstance(value, str) or len(value) != 64:
        raise ProfileError(f"{label} must be a SHA-256 hex string")
    if any(character not in "0123456789abcdef" for character in value):
        raise ProfileError(f"{label} is not lowercase hexadecimal")


def validate_distribution(value: Any, label: str) -> None:
    if not isinstance(value, dict):
        raise ProfileError(f"{label} must be an object")
    require_exact_keys(value, {"minimum", "maximum", "mean", "median", "p95"}, label)
    require_finite(value, label)
    if value["minimum"] > value["maximum"]:
        raise ProfileError(f"{label} minimum exceeds maximum")
    if not (value["minimum"] <= value["mean"] <= value["maximum"]):
        raise ProfileError(f"{label} mean is outside bounds")
    if not (value["minimum"] <= value["median"] <= value["maximum"]):
        raise ProfileError(f"{label} median is outside bounds")
    if not (value["minimum"] <= value["p95"] <= value["maximum"]):
        raise ProfileError(f"{label} p95 is outside bounds")


def validate_profile(profile: dict[str, Any], *, allow_empty_header_hash: bool = False) -> None:
    require_exact_keys(profile, PROFILE_KEYS, "profile")
    require_finite(profile, "profile")
    if profile["schema_version"] != 1:
        raise ProfileError("unsupported profile schema")
    for key, expected in EXPECTED_PINS.items():
        if profile[key] != expected:
            raise ProfileError(f"profile pin mismatch: {key}")
    if profile["simulation_frequency_hz"] != 20000:
        raise ProfileError("simulation frequency mismatch")
    if profile["sample_rate_hz"] != 44100:
        raise ProfileError("sample rate mismatch")
    if profile["deterministic_seed"] != 0x4E433033:
        raise ProfileError("deterministic seed mismatch")
    if not 0.0 <= profile["startup_throttle"] <= 1.0:
        raise ProfileError("startup throttle outside [0, 1]")
    criterion = profile["starter_disengagement_criterion"]
    if not isinstance(criterion, dict):
        raise ProfileError("starter disengagement criterion must be an object")
    require_exact_keys(criterion, {"type", "rpm"}, "starter criterion")
    if criterion["type"] != "rpm_at_or_above" or criterion["rpm"] <= 0.0:
        raise ProfileError("unsupported starter disengagement criterion")
    if profile["post_starter_minimum_rpm"] <= 0.0:
        raise ProfileError("post-starter minimum RPM must be positive")
    if profile["stability_window_seconds"] <= 0.0:
        raise ProfileError("stability window must be positive")
    if profile["maximum_startup_simulation_seconds"] <= 0.0:
        raise ProfileError("maximum startup time must be positive")

    sweep = profile["candidate_sweep"]
    if not isinstance(sweep, dict):
        raise ProfileError("candidate sweep must be an object")
    require_exact_keys(
        sweep,
        {
            "path",
            "sha256",
            "evaluated_candidate_count",
            "startup_throttle",
            "starter_disengagement_rpm",
            "post_starter_minimum_rpm",
            "stability_window_seconds",
            "maximum_startup_simulation_seconds",
        },
        "candidate sweep",
    )
    validate_sha(sweep["sha256"], "candidate sweep SHA")
    if sweep["evaluated_candidate_count"] != 1080:
        raise ProfileError("candidate sweep count must be 1080")
    if profile["startup_throttle"] not in sweep["startup_throttle"]:
        raise ProfileError("startup throttle was not measured")
    if criterion["rpm"] not in sweep["starter_disengagement_rpm"]:
        raise ProfileError("disengagement RPM was not measured")
    if profile["post_starter_minimum_rpm"] not in sweep["post_starter_minimum_rpm"]:
        raise ProfileError("minimum RPM was not measured")
    if profile["stability_window_seconds"] not in sweep["stability_window_seconds"]:
        raise ProfileError("stability window was not measured")
    if (
        profile["maximum_startup_simulation_seconds"]
        not in sweep["maximum_startup_simulation_seconds"]
    ):
        raise ProfileError("maximum startup time was not measured")

    trial_count = profile["selected_trial_count"]
    if not isinstance(trial_count, int) or trial_count < 10:
        raise ProfileError("selected trial count must be at least 10")
    if profile["success_count"] != trial_count or profile["failure_count"] != 0:
        raise ProfileError("selected candidate must have 100% success")
    traces = profile["positive_traces"]
    if not isinstance(traces, list) or len(traces) != trial_count:
        raise ProfileError("positive trace count mismatch")
    for index, trace in enumerate(traces):
        if not isinstance(trace, dict):
            raise ProfileError(f"positive trace {index} must be an object")
        require_exact_keys(trace, {"trial_index", "path", "sha256", "result"}, f"trace {index}")
        if trace["trial_index"] != index or trace["result"] != "Success":
            raise ProfileError("positive trace ordering/result mismatch")
        validate_sha(trace["sha256"], f"positive trace {index} SHA")

    for label, expected_result in (
        ("timeout_trace", "Timeout"),
        ("stall_trace", "StallAfterDisengagement"),
    ):
        trace = profile[label]
        if not isinstance(trace, dict):
            raise ProfileError(f"{label} must be an object")
        require_exact_keys(trace, {"path", "sha256", "result"}, label)
        if trace["result"] != expected_result:
            raise ProfileError(f"{label} result mismatch")
        validate_sha(trace["sha256"], f"{label} SHA")

    distributions = profile["observed_distributions"]
    if not isinstance(distributions, dict):
        raise ProfileError("observed distributions must be an object")
    expected_distributions = {
        "disengagement_time_seconds",
        "disengagement_rpm",
        "minimum_post_starter_rpm",
        "mean_post_starter_rpm",
        "produced_native_frames",
        "produced_pcm_frames",
        "post_start_pcm_rms",
    }
    require_exact_keys(distributions, expected_distributions, "observed distributions")
    for key in sorted(expected_distributions):
        validate_distribution(distributions[key], f"distribution {key}")

    margins = profile["selected_margins"]
    requirements = profile["selection_requirements"]
    if not isinstance(margins, dict) or not isinstance(requirements, dict):
        raise ProfileError("margins and requirements must be objects")
    require_exact_keys(
        margins,
        {
            "post_starter_rpm",
            "timeout_seconds",
            "disengagement_separation_rpm",
        },
        "selected margins",
    )
    require_exact_keys(
        requirements,
        {
            "minimum_selected_trials",
            "required_success_fraction",
            "minimum_post_starter_rpm_margin",
            "minimum_timeout_margin_seconds",
            "minimum_disengagement_separation_rpm",
        },
        "selection requirements",
    )
    if requirements["minimum_selected_trials"] < 10:
        raise ProfileError("minimum selected trials is below 10")
    if requirements["required_success_fraction"] != 1.0:
        raise ProfileError("required success fraction must be 1.0")
    if margins["post_starter_rpm"] < requirements["minimum_post_starter_rpm_margin"]:
        raise ProfileError("post-starter RPM margin is insufficient")
    if margins["timeout_seconds"] < requirements["minimum_timeout_margin_seconds"]:
        raise ProfileError("timeout margin is insufficient")
    if (
        margins["disengagement_separation_rpm"]
        < requirements["minimum_disengagement_separation_rpm"]
    ):
        raise ProfileError("disengagement separation is insufficient")

    generator = profile["generator"]
    verifier = profile["verifier"]
    header = profile["generated_header"]
    if not isinstance(generator, dict) or not isinstance(verifier, dict) or not isinstance(header, dict):
        raise ProfileError("tool/header records must be objects")
    require_exact_keys(generator, {"path", "sha256", "command"}, "generator")
    require_exact_keys(verifier, {"path", "sha256"}, "verifier")
    require_exact_keys(header, {"path", "sha256"}, "generated header")
    if generator["path"] != GENERATOR_PATH or generator["command"] != GENERATOR_COMMAND:
        raise ProfileError("generator path or command is not frozen")
    if header["path"] != HEADER_PATH:
        raise ProfileError("generated header path mismatch")
    validate_sha(generator["sha256"], "generator SHA")
    validate_sha(verifier["sha256"], "verifier SHA")
    validate_sha(
        header["sha256"],
        "generated header SHA",
        allow_empty=allow_empty_header_hash,
    )
    if profile["no_runtime_wav_dependency"] is not True:
        raise ProfileError("runtime WAV dependency must be false")
    if profile["calibration_status"] not in {
        "windows_pass_candidate",
        "windows_pass",
    }:
        raise ProfileError("invalid calibration status")
    validate_sha(profile["payload_sha256"], "payload SHA")
    if profile["payload_sha256"] != canonical_payload_sha256(profile):
        raise ProfileError("profile payload SHA mismatch")


def cpp_string(value: str) -> str:
    return (
        value.replace("\\", "\\\\")
        .replace('"', '\\"')
        .replace("\n", "\\n")
    )


def render_header(profile: dict[str, Any]) -> bytes:
    validate_profile(profile, allow_empty_header_hash=True)
    criterion = profile["starter_disengagement_criterion"]
    body = f"""// Generated deterministically from the measured Subaru EJ25 cold-start profile.
// Do not edit.
#pragma once

#include <cstdint>

namespace NextcarEngineSim::Generated {{

inline constexpr char SubaruEJ25ColdStartProfileId[] =
    "{cpp_string(profile['profile_id'])}";
inline constexpr char SubaruEJ25ColdStartProfileFixtureId[] =
    "{cpp_string(profile['fixture_id'])}";
inline constexpr char SubaruEJ25ColdStartProfileValidatedParitySourceSha[] =
    "{cpp_string(profile['validated_parity_source_sha'])}";
inline constexpr char SubaruEJ25ColdStartProfileEngineSimCommit[] =
    "{cpp_string(profile['engine_sim_commit'])}";
inline constexpr char SubaruEJ25ColdStartProfileEngineSimTree[] =
    "{cpp_string(profile['engine_sim_tree'])}";
inline constexpr char SubaruEJ25ColdStartProfileSolverCommit[] =
    "{cpp_string(profile['solver_commit'])}";
inline constexpr char SubaruEJ25ColdStartProfileFixtureSemanticContractSha256[] =
    "{cpp_string(profile['fixture_semantic_contract_sha256'])}";
inline constexpr char SubaruEJ25ColdStartProfileIrWavSha256[] =
    "{cpp_string(profile['ir_wav_sha256'])}";
inline constexpr char SubaruEJ25ColdStartProfileGeneratedIrSha256[] =
    "{cpp_string(profile['generated_ir_sha256'])}";
inline constexpr char SubaruEJ25ColdStartProfilePayloadSha256[] =
    "{cpp_string(profile['payload_sha256'])}";
inline constexpr char SubaruEJ25ColdStartProfileSelectedCandidateId[] =
    "{cpp_string(profile['selected_candidate_id'])}";

inline constexpr std::uint32_t SubaruEJ25ColdStartProfileSeed =
    {profile['deterministic_seed']}u;
inline constexpr int SubaruEJ25ColdStartProfileSimulationFrequencyHz =
    {profile['simulation_frequency_hz']};
inline constexpr int SubaruEJ25ColdStartProfileSampleRateHz =
    {profile['sample_rate_hz']};
inline constexpr double SubaruEJ25ColdStartProfileStartupThrottle =
    {profile['startup_throttle']:.17g};
inline constexpr double SubaruEJ25ColdStartProfileStarterDisengagementRpm =
    {criterion['rpm']:.17g};
inline constexpr double SubaruEJ25ColdStartProfilePostStarterMinimumRpm =
    {profile['post_starter_minimum_rpm']:.17g};
inline constexpr double SubaruEJ25ColdStartProfileStabilityWindowSeconds =
    {profile['stability_window_seconds']:.17g};
inline constexpr double SubaruEJ25ColdStartProfileMaximumStartupSimulationSeconds =
    {profile['maximum_startup_simulation_seconds']:.17g};
inline constexpr int SubaruEJ25ColdStartProfileTrialCount =
    {profile['selected_trial_count']};
inline constexpr int SubaruEJ25ColdStartProfileSuccessCount =
    {profile['success_count']};

static_assert(SubaruEJ25ColdStartProfileTrialCount >= 10);
static_assert(
    SubaruEJ25ColdStartProfileSuccessCount
    == SubaruEJ25ColdStartProfileTrialCount);

}} // namespace NextcarEngineSim::Generated
"""
    return body.encode("utf-8")


def atomic_write(path: Path, data: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary_name: str | None = None
    try:
        with tempfile.NamedTemporaryFile(
            mode="wb",
            dir=path.parent,
            prefix=f".{path.name}.",
            suffix=".tmp",
            delete=False,
        ) as handle:
            temporary_name = handle.name
            handle.write(data)
            handle.flush()
            os.fsync(handle.fileno())
        os.replace(temporary_name, path)
        temporary_name = None
    finally:
        if temporary_name is not None:
            try:
                os.unlink(temporary_name)
            except FileNotFoundError:
                pass


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--profile", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument(
        "--allow-empty-header-hash",
        action="store_true",
        help="allow the profile/header two-pass generation bootstrap",
    )
    args = parser.parse_args(argv)
    try:
        profile = load_profile(args.profile)
        validate_profile(
            profile,
            allow_empty_header_hash=args.allow_empty_header_hash,
        )
        atomic_write(args.output, render_header(profile))
    except (OSError, ProfileError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
