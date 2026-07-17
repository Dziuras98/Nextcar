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
    ROOT / ".github" / "pull_request_template.md",
    ROOT / ".github" / "workflows" / "repository-validation.yml",
)

REQUIRED_POLICY_PHRASES = (
    "Base every change on verified facts",
    "If any material requirement",
    "Every change must have an appropriate validation plan",
    "`main` is the sole integration branch",
    "Merge the pull request immediately after all required tests pass",
)

TEXT_SUFFIXES = {".cpp", ".h", ".hpp", ".cs", ".ini", ".md", ".py", ".yml", ".yaml"}
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
    workflow_path = ROOT / ".github" / "workflows" / "repository-validation.yml"
    workflow = workflow_path.read_text(encoding="utf-8")
    required_fragments = (
        "pull_request:",
        "main",
        "python scripts/validate_repository.py",
    )
    for fragment in required_fragments:
        if fragment not in workflow:
            fail(f"Validation workflow is missing: {fragment!r}")


def main() -> int:
    checks = (
        check_required_files,
        check_policy_content,
        check_json_files,
        check_balanced_delimiters,
        check_unreal_generated_header_order,
        check_utf8_text_files,
        check_workflow_scope,
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
