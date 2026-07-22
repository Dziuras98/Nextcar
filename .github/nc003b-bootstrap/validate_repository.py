#!/usr/bin/env python3
"""Run deterministic repository-level validation without external packages."""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

REQUIRED_FILES = (
    ROOT / "AGENTS.md",
    ROOT / "docs" / "manager-history.md",
    ROOT / ".github" / "pull_request_template.md",
    ROOT / ".github" / "workflows" / "repository-validation.yml",
    ROOT / ".github" / "workflows" / "delete-merged-branch.yml",
    ROOT / "Tools" / "EngineSimVendor" / "FixtureSourceSnapshot" / "SNAPSHOT.json",
    ROOT / "Tools" / "EngineSimVendor" / "verify_fixture_source_snapshot.py",
)

REQUIRED_POLICY_PHRASES = (
    "Base every change on verified facts",
    "If any material requirement",
    "Every change must have an appropriate validation plan",
    "When an agent is explicitly instructed to act as the manager",
    "read and analyze the entire `AGENTS.md`",
    "dispatch as many programmer agents as can safely work in parallel",
    "`docs/manager-history.md` is the canonical, append-only record",
    "A manager must not claim the task is complete",
    "`main` is the sole integration branch",
    "Merge the pull request immediately after all required tests pass",
    "automatically delete its source branch",
)

REQUIRED_MANAGER_HISTORY_PHRASES = (
    "# Manager history",
    "## Required entry format",
    "Repository history reviewed:",
    "Workstream decomposition and programmer-agent assignments:",
    "Evidence and exact tests:",
    "Unresolved risks or blockers:",
)

REQUIRED_PR_TEMPLATE_PHRASES = (
    "## Manager record",
    "complete repository history and all previous notes",
    "`docs/manager-history.md` contains a final append-only entry",
)

TEXT_SUFFIXES = {".cpp", ".h", ".hpp", ".cs", ".ini", ".inc", ".md", ".ps1", ".ps1frag", ".py", ".yml", ".yaml"}
BALANCED_SUFFIXES = {".cpp", ".h", ".hpp", ".cs"}
JSON_SUFFIXES = {".json", ".uproject", ".uplugin"}


def fail(message: str) -> None:
    raise AssertionError(message)


def check_required_files() -> None:
    for path in REQUIRED_FILES:
        if not path.is_file():
            fail(f"Missing required file: {path.relative_to(ROOT)}")


def check_policy_content() -> None:
    policy = (ROOT / "AGENTS.md").read_text(encoding="utf-8")
    for phrase in REQUIRED_POLICY_PHRASES:
        if phrase not in policy:
            fail(f"AGENTS.md is missing required policy text: {phrase!r}")


def check_manager_history_content() -> None:
    history = (ROOT / "docs" / "manager-history.md").read_text(encoding="utf-8")
    for phrase in REQUIRED_MANAGER_HISTORY_PHRASES:
        if phrase not in history:
            fail(f"Manager history is missing required text: {phrase!r}")


def check_pull_request_template_content() -> None:
    template = (ROOT / ".github" / "pull_request_template.md").read_text(encoding="utf-8")
    for phrase in REQUIRED_PR_TEMPLATE_PHRASES:
        if phrase not in template:
            fail(f"Pull request template is missing required text: {phrase!r}")


def check_json_files() -> None:
    for path in sorted(ROOT.rglob("*")):
        if path.is_file() and path.suffix.lower() in JSON_SUFFIXES:
            try:
                json.loads(path.read_text(encoding="utf-8"))
            except (UnicodeDecodeError, json.JSONDecodeError) as exc:
                fail(f"Invalid JSON in {path.relative_to(ROOT)}: {exc}")


def strip_comments_and_literals(source: str) -> str:
    pattern = re.compile(
        r"//[^\n]*|/\*.*?\*/|\"(?:\\.|[^\"\\])*\"|'(?:\\.|[^'\\])*'",
        re.DOTALL,
    )
    return pattern.sub("", source)


def check_balanced_delimiters() -> None:
    pairs = {"}": "{", ")": "(", "]": "["}
    openings = set(pairs.values())

    for path in sorted(ROOT.rglob("*")):
        if not path.is_file() or path.suffix.lower() not in BALANCED_SUFFIXES:
            continue

        cleaned = strip_comments_and_literals(path.read_text(encoding="utf-8"))
        stack: list[tuple[str, int]] = []
        for index, character in enumerate(cleaned):
            if character in openings:
                stack.append((character, index))
            elif character in pairs:
                if not stack or stack[-1][0] != pairs[character]:
                    fail(
                        f"Unbalanced delimiter {character!r} in "
                        f"{path.relative_to(ROOT)} at character {index}"
                    )
                stack.pop()

        if stack:
            character, index = stack[-1]
            fail(
                f"Unclosed delimiter {character!r} in "
                f"{path.relative_to(ROOT)} at character {index}"
            )


def check_unreal_generated_header_order() -> None:
    for path in sorted(ROOT.rglob("*.h")):
        lines = path.read_text(encoding="utf-8").splitlines()
        include_lines = [
            (index, line.strip())
            for index, line in enumerate(lines)
            if line.strip().startswith("#include")
        ]
        generated = [item for item in include_lines if ".generated.h\"" in item[1]]

        if not generated:
            continue
        if len(generated) != 1:
            fail(f"Expected one generated header include in {path.relative_to(ROOT)}")
        if include_lines[-1] != generated[0]:
            fail(
                f"The .generated.h include must be the final include in "
                f"{path.relative_to(ROOT)}"
            )


def check_utf8_text_files() -> None:
    for path in sorted(ROOT.rglob("*")):
        if not path.is_file() or path.suffix.lower() not in TEXT_SUFFIXES:
            continue
        try:
            path.read_text(encoding="utf-8")
        except UnicodeDecodeError as exc:
            fail(f"Non-UTF-8 text file {path.relative_to(ROOT)}: {exc}")


def check_workflow_scope() -> None:
    validation_path = ROOT / ".github" / "workflows" / "repository-validation.yml"
    validation = validation_path.read_text(encoding="utf-8")
    for fragment in ("pull_request:", "main", "python scripts/validate_repository.py"):
        if fragment not in validation:
            fail(f"Validation workflow is missing: {fragment!r}")

    cleanup_path = ROOT / ".github" / "workflows" / "delete-merged-branch.yml"
    cleanup = cleanup_path.read_text(encoding="utf-8")
    required_cleanup_fragments = (
        "types:",
        "closed",
        "merged == true",
        "head.repo.full_name == github.repository",
        "head.ref != 'main'",
        "head.ref != 'master'",
        "contents: write",
        "git.deleteRef",
    )
    for fragment in required_cleanup_fragments:
        if fragment not in cleanup:
            fail(f"Branch cleanup workflow is missing: {fragment!r}")



def check_phase0_closure() -> None:
    sourceinputs = ROOT / "Tools" / "EngineSimVendor" / "SourceInputs"
    if sourceinputs.exists():
        fail("Temporary Tools/EngineSimVendor/SourceInputs must be absent")
    for name in ("RECONSTRUCTION_AUDIT.md", "WIP_CHECKPOINT.md"):
        if (ROOT / "Tools" / "EngineSimVendor" / name).exists():
            fail(f"Reconstruction-only document remains: {name}")
    forbidden_names = {"_transport_probe_local.txt", "_should_not_use", "RUNNER_BOOTSTRAP_REQUEST", "bootstrap_on_runner.ps1"}
    vendor_root = ROOT / "Tools" / "EngineSimVendor"
    core_root = (
        ROOT
        / "Plugins"
        / "NextcarEngineSim"
        / "Source"
        / "NextcarEngineSimCore"
    )
    phase0_roots = (vendor_root, core_root)
    forbidden_cold_start_sources = vendor_root / "_cold_start_sources"
    if forbidden_cold_start_sources.exists():
        fail(f"Closure residue remains: {forbidden_cold_start_sources.relative_to(ROOT)}")

    def is_phase0_path(path: Path) -> bool:
        return any(path == root or root in path.parents for root in phase0_roots)

    for path in sorted(ROOT.rglob("*")):
        relative = path.relative_to(ROOT)
        if path.name in forbidden_names or path.suffix == ".pyc" or "__pycache__" in path.parts:
            fail(f"Closure residue remains: {relative}")
        if not is_phase0_path(path):
            continue
        if path.name == "validate_cold_start_windows.parts":
            fail(f"Closure residue remains: {relative}")
        if path.is_file() and path.name.lower().endswith(".ps1frag"):
            fail(f"Closure residue remains: {relative}")
        if path.is_file() and re.search(r"\.part\d{2}\.inc\Z", path.name, re.IGNORECASE):
            fail(f"Closure residue remains: {relative}")
        if path.is_file() and vendor_root in path.parents and path.name.lower().endswith(".py.gz"):
            fail(f"Closure residue remains: {relative}")

    for path in sorted(vendor_root.rglob("*.py")):
        source = path.read_text(encoding="utf-8")
        if (
            re.search(r"gzip\s*\.\s*decompress", source)
            and re.search(r"exec\s*\(\s*compile\s*\(", source)
        ):
            fail(f"Dynamic Python launcher remains: {path.relative_to(ROOT)}")

    scriptblock_create = re.compile(
        r"(?:\[[^]\r\n]*ScriptBlock\]|ScriptBlock)\s*::\s*Create\b",
        re.IGNORECASE,
    )
    for path in sorted(vendor_root.rglob("*.ps1")):
        if scriptblock_create.search(path.read_text(encoding="utf-8")):
            fail(f"Dynamic PowerShell ScriptBlock launcher remains: {path.relative_to(ROOT)}")
    workflow = ROOT / ".github" / "workflows" / "nc003b-phase0-final-closure-windows.yml"
    if workflow.exists():
        fail("Temporary final-closure workflow must not be present in the candidate tree")
    snapshot_index = ROOT / "Tools" / "EngineSimVendor" / "FixtureSourceSnapshot" / "SNAPSHOT.json"
    snapshot = json.loads(snapshot_index.read_text(encoding="utf-8"))
    if snapshot.get("file_count") != 4 or len(snapshot.get("files", [])) != 4:
        fail("Fixture source snapshot must contain exactly four indexed files")
    manifest = json.loads((ROOT / "Tools" / "EngineSimVendor" / "SOURCE_MANIFEST.json").read_text(encoding="utf-8"))
    if manifest.get("schema_version") != 1 or not isinstance(manifest.get("files"), list):
        fail("SOURCE_MANIFEST.json must remain a monolithic schema_version 1 document")
    if "fixture_source_snapshot" not in manifest or "final_closure" not in manifest:
        fail("SOURCE_MANIFEST.json lacks Phase 0 closure sections")

def main() -> int:
    checks = (
        check_required_files,
        check_policy_content,
        check_manager_history_content,
        check_pull_request_template_content,
        check_json_files,
        check_balanced_delimiters,
        check_unreal_generated_header_order,
        check_utf8_text_files,
        check_workflow_scope,
        check_phase0_closure,
    )

    for check in checks:
        check()
        print(f"PASS: {check.__name__}")

    print(f"Repository validation passed ({len(checks)} checks).")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        raise SystemExit(1)
