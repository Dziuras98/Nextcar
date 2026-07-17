# Manager history

This file is the canonical, append-only record of manager activity, repository-state analysis, delegation, integration decisions, validation, and handoff notes. A manager must read the entire file at the start of every manager run and append one final entry before declaring the run complete.

## Required entry format

Each new entry must contain all of the following fields:

- Timestamp:
- User request:
- Baseline branch and commit:
- Repository history reviewed:
- Repository state found:
- Workstream decomposition and programmer-agent assignments:
- Files and behavior changed:
- Evidence and exact tests:
- Decisions and integration notes:
- Unresolved risks or blockers:
- Next steps:

Do not delete, reorder, or rewrite earlier entries. Append corrections as new entries that identify the entry being corrected.

## 2026-07-17 — Manager orchestration policy bootstrap

- Timestamp: 2026-07-17 (Europe/Warsaw)
- User request: Add instructions defining the manager role as repository reviewer, state analyst, change documenter, and dispatcher of the maximum safe number of parallel programmer agents; require full history and note review and persistent logging on every run.
- Baseline branch and commit: `main` at `02eecc2afbb0e0599692836e25b86d1672181da4`.
- Repository history reviewed:
  - `07664bb53fc0bf999796b924d98e9b6fd3fd0439` — initialized the Nextcar repository and README.
  - `bc33ceec837a26dab136f63a68a6c7d17fc507e6` — established `main`, repository execution policy, PR evidence checklist, validation script, and validation workflow.
  - `ecac2fc9f05336ee2f7c3a7b8e7c0cab7161ceeb` — accidentally added temporary file `tmp`.
  - `f26fad190f9871f36e827e05a8b77a8e8f48f03c` — removed the accidental temporary file.
  - `02eecc2afbb0e0599692836e25b86d1672181da4` — added automatic deletion of successfully merged same-repository work branches and corresponding policy validation.
- Repository state found: The repository had a global `AGENTS.md`, PR template, deterministic repository validator, and branch-cleanup workflow, but no canonical manager-history file or manager-specific delegation requirements.
- Workstream decomposition and programmer-agent assignments: The policy update was divided into manager-role rules, persistent-history format, PR checklist enforcement, and deterministic validation enforcement. These files share repository-policy invariants and were integrated as one coordinated change.
- Files and behavior changed: Added manager orchestration and parallel-delegation requirements to `AGENTS.md`; created this append-only history; added manager checks to the PR template; extended `scripts/validate_repository.py` to require and validate the policy, history, and PR checklist.
- Evidence and exact tests: `python scripts/validate_repository.py` — passed all 9 checks in a reconstructed complete policy-file workspace before publication. GitHub Actions validation must also pass on the pull request before merge.
- Decisions and integration notes: `docs/manager-history.md` is the canonical persistent record. Every manager run must read it in full and append a final entry, including blocked or analysis-only runs. Parallel agents must have non-overlapping ownership unless an explicit integration plan exists.
- Unresolved risks or blockers: The textual validator can enforce required policy and history structure, but cannot prove that a manager actually read every historical commit or maximized safe parallelism; PR review and manager accountability remain required.
- Next steps: Future manager agents must load the full reachable Git history and this entire file before planning, then append their own final entry in the same managed change.
