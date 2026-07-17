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

## 2026-07-17 — Vertical-slice roadmap and integration-gate review

- Timestamp: 2026-07-17 12:33 (Europe/Warsaw)
- User request: Use the `Nextcar` repository, act as project manager, and propose the next development steps.
- Baseline branch and commit: `main` at `5d692320c0bc96e21c81d0307095bc96d567d35d`.
- Repository history reviewed:
  - The complete six-commit `main` history available through the GitHub connector was reviewed: repository initialization; integration-branch policy; accidental temporary file addition and removal; merged-branch cleanup; and manager orchestration/history policy.
  - Open draft PR #1 (`agent/initial-driving-prototype` at `d44d4fdf5955aa0e17f2861470edcf97fb5a9fba`) was reviewed through its complete final diff, changed-file list, PR description, CI state, and comparison against current `main`.
  - The PR branch is 37 commits ahead and 5 commits behind current `main`, with merge base `07664bb53fc0bf999796b924d98e9b6fd3fd0439`. The available connector does not expose the individual PR commit list, so individual inspection of all 37 commit objects remains an explicit tooling limitation.
  - The related `Dziuras98/engine-sim` fork was inspected at its default branch for build structure, core/runtime separation, audio-output API, dependencies, and license.
- Repository state found:
  - `main` contains repository policy, validation, cleanup automation, README, and manager history, but the playable Unreal prototype is still isolated in draft PR #1.
  - PR #1 adds a code-only Unreal Engine 5.8 project, kinematic arcade vehicle, infinite test ground, HUD, portable deterministic simulation, standalone tests, and opt-in self-hosted Unreal CI.
  - Repository validation and hosted GCC, Clang, and UBSan vehicle tests pass. Full Unreal Engine CI is skipped because no enabled Windows Unreal Engine 5.8 self-hosted runner has compiled `NextcarEditor` or run Unreal Automation Tests.
  - The `engine-sim` fork already defines the simulation core as a static C++ library and exposes PCM audio through `Simulator::readAudioOutput`, but its documented complete application build is Windows-only and includes optional scripting/UI dependencies that should not be copied wholesale into the UE runtime.
- Workstream decomposition and programmer-agent assignments:
  - This run was analysis and change-control only; no programmer agent was dispatched because the user requested a proposal rather than feature implementation and no independent code change was authorized.
  - The next execution should use three sequentially gated phases. After PR #1 is validated and merged, the engine integration spike can safely split into three parallel non-overlapping workstreams: core-library extraction/build, Unreal procedural-audio adapter, and benchmark/test harness. Vehicle/powertrain coupling follows only after those interfaces are proven.
- Files and behavior changed: Appended this manager record only. No gameplay, build, CI, or engine-simulation behavior was changed.
- Evidence and exact tests:
  - Inspected `AGENTS.md`, the complete prior `docs/manager-history.md`, repository metadata, all visible `main` commits, all PRs and issues, PR #1 diff and changed files, CI results, selected vehicle production/test headers, `Nextcar.Build.cs`, and relevant `engine-sim` README, license, CMake, simulator, piston simulator, and synthesizer interfaces.
  - `python scripts/validate_repository.py` — passed all 9 checks in a reconstructed workspace before publication; GitHub Actions validation is required on this history-only pull request.
  - No Unreal build is required for this history-only Markdown change.
- Decisions and integration notes:
  - The immediate project gate is to compile and validate PR #1 with Unreal Engine 5.8, update it against `main`, and merge it before starting engine-audio integration.
  - The first `engine-sim` deliverable should be a headless, benchmarked core producing stable PCM for one fixed engine configuration. Direct UE integration should use a narrow wrapper and Unreal procedural audio, not the original SDL/UI application.
  - RPM, throttle/load, clutch, gear, and driveline state must be represented by explicit runtime interfaces; the temporary arcade speed model must not become the permanent powertrain contract.
- Unresolved risks or blockers:
  - No Windows Unreal Engine 5.8 self-hosted runner is enabled, so the UE-facing prototype remains uncompiled.
  - The open prototype branch has diverged from `main` and must be integrated carefully before merge.
  - Individual commit objects for all 37 commits in the PR branch could not be enumerated through the available connector; final-state diff and comparison were reviewed instead.
  - Real-time CPU cost, audio underrun behavior, thread safety, engine configuration loading, and third-party dependency packaging for `engine-sim` are not yet measured in the UE environment.
- Next steps:
  - Gate 0: enable or locally execute Unreal Engine 5.8 build and automation tests for PR #1, resolve failures, update the branch from `main`, perform a short manual driving smoke test, and merge.
  - Gate 1: create a headless `engine-sim` integration spike that builds the minimal static core, loads one engine, accepts throttle/load targets, emits PCM, and records real-time factor, callback time, buffer fill, and underruns.
  - Gate 2: add an Unreal plugin/module with a worker-thread simulation wrapper and procedural audio source; prove clean start/stop, RPM response, and glitch-free output before coupling it to vehicle dynamics.
  - Gate 3: introduce explicit engine, clutch, gearbox, and driveline state; replace direct throttle-to-speed behavior progressively while preserving the existing deterministic vehicle tests.
  - Gate 4: construct the first vertical slice with one car, one engine configuration, simple test map, telemetry HUD, performance budgets, and repeatable acceptance tests.
