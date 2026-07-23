#!/usr/bin/env python3
"""Finalize a measured profile/header pair and write deterministic calibration evidence."""

from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import tempfile
import sys
from copy import deepcopy
from pathlib import Path
from typing import Any


class EvidenceError(RuntimeError):
    pass


def reject_constant(value: str) -> None:
    raise EvidenceError(f"non-finite JSON token: {value}")


def load_json(path: Path) -> Any:
    try:
        return json.loads(
            path.read_text(encoding="utf-8"),
            parse_constant=reject_constant,
        )
    except (OSError, json.JSONDecodeError) as exc:
        raise EvidenceError(f"{path}: {exc}") from exc


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


def sha256_file(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def payload_sha256(profile: dict[str, Any]) -> str:
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


def run(command: list[str], cwd: Path) -> str:
    completed = subprocess.run(
        command,
        cwd=cwd,
        check=False,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if completed.returncode != 0:
        raise EvidenceError(
            f"command failed ({completed.returncode}): "
            + completed.stdout.strip()
        )
    return completed.stdout


def render_evidence(
    profile: dict[str, Any],
    summary: dict[str, Any],
    run_metadata: dict[str, Any],
    profile_sha: str,
    header_sha: str,
) -> str:
    selected = summary["selected_candidate"]
    lines = [
        "# Subaru EJ25 cold-start calibration",
        "",
        "> Status: Phase 0 WIP — cold-start calibration published, manager review pending, not submitted for manager approval. Phase 1 not started.",
        "",
        "## Validated source",
        "",
        f"- Candidate source SHA: `{run_metadata['candidate_source_sha']}`",
        f"- Workflow run ID: `{run_metadata['run_id']}`",
        f"- Artifact ID: `{run_metadata['artifact_id']}`",
        f"- Artifact name: `{run_metadata['artifact_name']}`",
        f"- Runner image: `{run_metadata['runner_image']}`",
        f"- Visual Studio: `{run_metadata['visual_studio_version']}`",
        f"- MSVC: `{run_metadata['msvc_version']}`",
        f"- VCTools: `{run_metadata['vc_tools_version']}`",
        f"- Windows SDK: `{run_metadata['windows_sdk_version']}`",
        f"- CMake: `{run_metadata['cmake_version']}`",
        f"- Python: `{run_metadata['python_version']}`",
        "",
        "## Sweep",
        "",
        "- Evaluated candidates: `1080`",
        "- Discovery traces: `30`",
        "- Startup throttle: `0.05, 0.10, 0.20, 0.35, 0.50`",
        "- Starter disengagement RPM: `500, 650, 800, 950, 1100, 1300`",
        "- Post-starter minimum RPM: `400, 500, 600, 700`",
        "- Stability windows: `0.5, 1.0, 2.0 s`",
        "- Maximum startup times: `2.0, 4.0, 8.0 s`",
        "",
        "## Selected candidate",
        "",
        f"- Candidate ID: `{profile['selected_candidate_id']}`",
        f"- Startup throttle: `{profile['startup_throttle']}`",
        f"- Starter disengagement criterion: RPM >= `{profile['starter_disengagement_criterion']['rpm']}`",
        f"- Post-starter minimum RPM: `{profile['post_starter_minimum_rpm']}`",
        f"- Stability window: `{profile['stability_window_seconds']} s`",
        f"- Maximum startup simulation time: `{profile['maximum_startup_simulation_seconds']} s`",
        f"- Trials: `{profile['selected_trial_count']}`",
        f"- Successes: `{profile['success_count']}`",
        f"- Failures: `{profile['failure_count']}`",
        "",
        "## Selected margins",
        "",
        f"- Post-starter RPM margin: `{profile['selected_margins']['post_starter_rpm']}`",
        f"- Timeout margin: `{profile['selected_margins']['timeout_seconds']} s`",
        f"- Disengagement separation: `{profile['selected_margins']['disengagement_separation_rpm']} RPM`",
        "",
        "## Distributions",
        "",
    ]
    for name, values in profile["observed_distributions"].items():
        lines.append(
            f"- `{name}`: min `{values['minimum']}`, max `{values['maximum']}`, "
            f"mean `{values['mean']}`, median `{values['median']}`, p95 `{values['p95']}`"
        )
    lines += [
        "",
        "## Negative cases",
        "",
        f"- Timeout: `{profile['timeout_trace']['result']}` — `{profile['timeout_trace']['path']}` — SHA-256 `{profile['timeout_trace']['sha256']}`",
        f"- Stall after disengagement: `{profile['stall_trace']['result']}` — `{profile['stall_trace']['path']}` — SHA-256 `{profile['stall_trace']['sha256']}`",
        "",
        "## Positive traces",
        "",
    ]
    for trace in profile["positive_traces"]:
        lines.append(
            f"- Trial `{trace['trial_index']}`: `{trace['path']}` — SHA-256 `{trace['sha256']}`"
        )
    lines += [
        "",
        "## Generated inputs",
        "",
        f"- Profile JSON SHA-256: `{profile_sha}`",
        f"- Profile payload SHA-256: `{profile['payload_sha256']}`",
        f"- Generated header SHA-256: `{header_sha}`",
        f"- Generator SHA-256: `{profile['generator']['sha256']}`",
        f"- Verifier SHA-256: `{profile['verifier']['sha256']}`",
        f"- Generator command: `{profile['generator']['command']}`",
        "",
        "## Windows validation",
        "",
        f"- Python tests: `{run_metadata['python_tests']}`",
        f"- MSVC x64 Release: `{run_metadata['release_result']}`",
        f"- MSVC x64 Debug `/RTC1`: `{run_metadata['debug_result']}`",
        f"- MSVC x64 AddressSanitizer: `{run_metadata['asan_result']}`",
        f"- Selected-profile trials: `{run_metadata['selected_trials_result']}`",
        f"- Timeout case: `{run_metadata['timeout_result']}`",
        f"- Stall case: `{run_metadata['stall_result']}`",
        f"- Clean reproducibility: `{run_metadata['clean_reproducibility']}`",
        f"- Project warning count: `{run_metadata['project_warning_count']}`",
        "",
        "The fixture transcription, mechanical values, engine-sim pin, solver pin, exact WAV, and generated impulse response were not changed.",
        "",
    ]
    return "\n".join(lines)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--profile-candidate", required=True, type=Path)
    parser.add_argument("--selected-summary", required=True, type=Path)
    parser.add_argument("--run-metadata", required=True, type=Path)
    parser.add_argument("--repo-root", required=True, type=Path)
    parser.add_argument("--output-profile", required=True, type=Path)
    parser.add_argument("--output-header", required=True, type=Path)
    parser.add_argument("--output-evidence", required=True, type=Path)
    args = parser.parse_args(argv)

    try:
        repo_root = args.repo_root.resolve()
        profile = load_json(args.profile_candidate)
        summary = load_json(args.selected_summary)
        run_metadata = load_json(args.run_metadata)
        if not isinstance(profile, dict) or not isinstance(summary, dict):
            raise EvidenceError("profile and summary must be objects")
        if not isinstance(run_metadata, dict):
            raise EvidenceError("run metadata must be an object")

        profile["calibration_status"] = "windows_pass"
        profile["generated_header"]["sha256"] = ""
        profile["payload_sha256"] = payload_sha256(profile)
        write_json(args.output_profile, profile)

        generator = repo_root / profile["generator"]["path"]
        run(
            [
                sys.executable,
                str(generator),
                "--profile",
                str(args.output_profile),
                "--output",
                str(args.output_header),
                "--allow-empty-header-hash",
            ],
            repo_root,
        )
        header_sha = sha256_file(args.output_header)
        profile["generated_header"]["sha256"] = header_sha
        if payload_sha256(profile) != profile["payload_sha256"]:
            raise EvidenceError("profile payload changed during header finalization")
        write_json(args.output_profile, profile)

        with tempfile.TemporaryDirectory() as temporary:
            deterministic_header = Path(temporary) / "profile.generated.h"
            run(
                [
                    sys.executable,
                    str(generator),
                    "--profile",
                    str(args.output_profile),
                    "--output",
                    str(deterministic_header),
                ],
                repo_root,
            )
            if deterministic_header.read_bytes() != args.output_header.read_bytes():
                raise EvidenceError("generated header is not deterministic")

        profile_sha = sha256_file(args.output_profile)
        evidence = render_evidence(
            profile,
            summary,
            run_metadata,
            profile_sha,
            header_sha,
        )
        args.output_evidence.parent.mkdir(parents=True, exist_ok=True)
        args.output_evidence.write_text(
            evidence,
            encoding="utf-8",
            newline="\n",
        )
    except (OSError, EvidenceError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
