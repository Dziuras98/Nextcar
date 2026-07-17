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

## 2026-07-17 — Windows 11 Unreal runner runbook

- Timestamp: 2026-07-17 14:52 (Europe/Warsaw)
- User request: Prepare concrete instructions for running the project's local GitHub Actions runner on a Windows 11 computer.
- Baseline branch and commit: `main` at `0ddf61cb77e5edeb8bceab56877a592f8a49e158`.
- Repository history reviewed:
  - Reviewed the complete seven-commit `main` history available through the GitHub connector, including the two previous manager-history entries.
  - Re-read the complete current `AGENTS.md` and `docs/manager-history.md`.
  - Re-inspected open draft PR #1 and its `unreal-full-ci.yml` workflow, including trigger conditions, required runner labels, `ENABLE_UNREAL_CI`, build command, test command, report requirement, and artifact paths.
  - Reviewed current official GitHub documentation for Windows self-hosted runner registration, labels, services, variables, network requirements, and public-repository security; reviewed current Epic Unreal Engine 5.8 documentation for Windows, Visual Studio, SDK, and C++ toolchain requirements.
- Repository state found:
  - `Nextcar` remains public and PR #1 remains the only open gameplay PR requiring an Unreal Engine 5.8 Windows build.
  - The workflow requires labels `self-hosted`, `Windows`, `X64`, and `unreal-5.8`, and skips pull-request execution unless repository variable `ENABLE_UNREAL_CI` is `true`.
  - A permanently online self-hosted runner on this public repository would expose the Windows host to untrusted workflow code, so the current safe operating mode is a manually started interactive runner used only for reviewed same-repository changes.
- Workstream decomposition and programmer-agent assignments:
  - The task was decomposed into repository-specific workflow analysis, current GitHub runner/security requirements, current Unreal/Visual Studio prerequisites, PowerShell preflight commands, runner registration, workflow activation, shutdown, and troubleshooting.
  - No programmer agent was dispatched because the requested output is one cohesive operational runbook and the only writable files are shared documentation/history files requiring sequential integration.
- Files and behavior changed:
  - Added `docs/windows-self-hosted-runner.md` with Windows 11 prerequisites, `UE_ROOT` setup, exact local UBT and Unreal Automation Test preflight, runner registration and labels, safe interactive operation, `ENABLE_UNREAL_CI` activation, result verification, shutdown, optional service configuration, and troubleshooting.
  - Appended this manager-history entry. No gameplay, workflow, build, or simulation behavior was changed.
- Evidence and exact tests:
  - Verified the runbook against the current PR #1 workflow and official GitHub and Epic documentation.
  - Attempted `git clone --branch agent/nc-002-windows-runner-runbook --single-branch https://github.com/Dziuras98/Nextcar.git /tmp/nextcar-runner-runbook && python scripts/validate_repository.py`; the command stopped before checkout because the tool container could not resolve `github.com`. This is an environment/network blocker, not a repository validation result.
  - GitHub Actions `Repository validation` run 30 on commit `c20625feacd2d90ffcaaa28a05dea8aafd225ac8` completed successfully and executed the repository validator against the actual branch contents.
  - No Unreal build is required because this change affects Markdown documentation only.
- Decisions and integration notes:
  - For the current public repository, interactive `run.cmd` operation is the default; permanent Windows-service operation is documented only as an option after making the repository private or restricting workflow access to trusted code.
  - The runbook uses the current workflow contract rather than generic runner instructions, including the exact custom label, machine-level `UE_ROOT`, `NextcarEditor` build target, `Nextcar.*` tests, and report artifact paths.
- Unresolved risks or blockers:
  - The actual Windows machine, Unreal Engine installation path, Visual Studio installation, runner registration, and UE build have not yet been exercised by the user.
  - The current runner workflow remains on PR #1 rather than `main`; an old skipped run may require a new trusted PR synchronization after setting `ENABLE_UNREAL_CI=true`.
  - Persistent self-hosted execution remains unsafe while the repository is public and accepts pull requests containing executable workflow/project code.
- Next steps:
  - Complete the runbook on the Windows 11 machine through the local Unreal preflight.
  - Register `NEXTCAR-UE58` with the custom `unreal-5.8` label, start it interactively, enable `ENABLE_UNREAL_CI`, and run Full Unreal Engine CI for PR #1.
  - Preserve the automation report and logs, set `ENABLE_UNREAL_CI=false`, stop the runner, then use the build/test evidence to decide whether PR #1 can be updated and merged.

## 2026-07-17 — Private Windows runner service default

- Timestamp: 2026-07-17 15:29 (Europe/Warsaw)
- User request: The repository was changed to private; immediately update the Windows 11 runner instructions so the GitHub Actions runner is installed and operated as a Windows service.
- Baseline branch and commit: `main` at `48bcb7ed1f380ef67a60603f6e0c4d9d031d6d60`.
- Repository history reviewed:
  - Reviewed the complete eight-commit `main` history available through the GitHub connector, including the original runner runbook change and all earlier policy/history changes.
  - Re-read the complete current `AGENTS.md`, `docs/manager-history.md`, and `docs/windows-self-hosted-runner.md`.
  - Re-checked current GitHub documentation for adding a Windows self-hosted runner, installing it as a service, managing the service with PowerShell, and security limitations that still apply to private repositories.
- Repository state found:
  - GitHub repository metadata now reports `Dziuras98/Nextcar` visibility as `private`.
  - The existing runbook still described the repository as public, required interactive `run.cmd` operation, and treated Windows service installation as optional future work.
  - GitHub installs the Windows service during `config.cmd`; a runner already configured without the service must be removed and configured again.
  - Private visibility removes public-fork exposure but does not isolate self-hosted jobs from trusted collaborators or workflow code with repository access.
- Workstream decomposition and programmer-agent assignments:
  - The change was decomposed into security posture, machine-level environment, dedicated service identity, registration/reconfiguration, service verification, persistent operation, and service-specific troubleshooting.
  - No programmer agent was dispatched because the change is one cohesive operational procedure and both writable files are shared documentation/history files requiring sequential integration.
- Files and behavior changed:
  - Updated `docs/windows-self-hosted-runner.md` to make a persistent automatically started Windows service the default.
  - Added creation and permission setup for a dedicated non-administrator `NextcarRunner` account, service installation prompts, migration from an interactive runner, reboot verification, machine-level environment requirements, normal service operation, and service-specific diagnostics.
  - Appended this manager-history entry. No workflow, gameplay, build, or simulation code changed.
- Evidence and exact tests:
  - GitHub repository metadata confirmed `visibility: private` before editing.
  - The service procedure was checked against current official GitHub documentation: Windows service setup occurs during runner configuration; an existing non-service runner must be removed and reconfigured; service state can be managed through PowerShell.
  - The local environment does not have the GitHub CLI installed (`gh: command not found`), so it could not perform an authenticated clone of the private repository.
  - GitHub Actions `Repository validation` run 33 on commit `f08e7116818da4038cd2fc6e83157de7aae7b674` completed successfully and executed `python scripts/validate_repository.py` against the exact branch contents.
  - No Unreal build or Unreal Automation Test is required because the diff affects Markdown documentation only.
- Decisions and integration notes:
  - The runner service uses a dedicated unprivileged local account rather than `LocalSystem` or an administrator account.
  - `UE_ROOT` and command-line dependencies must be machine-level because Windows services do not reliably inherit an interactive user's environment.
  - The service remains installed and running after CI; `ENABLE_UNREAL_CI` remains the control for whether full Unreal jobs execute.
- Unresolved risks or blockers:
  - The procedure has not yet been exercised on the user's Windows 11 machine, so the exact service-account permissions required by the installed Unreal and Visual Studio locations remain to be verified.
  - Private collaborators with sufficient repository access can still submit workflow or build code that executes on the host; repository access and workflow changes must remain trusted and reviewed.
- Next steps:
  - Merge the documentation change after the final repository validation passes on the updated history commit.
  - On the Windows computer, complete the local preflight, create `NextcarRunner`, configure `NEXTCAR-UE58` as a service, reboot, and confirm the service returns to Running and GitHub reports the runner as Idle.
  - Set `ENABLE_UNREAL_CI=true` and run Full Unreal Engine CI for PR #1; preserve the automation report and logs.

## 2026-07-17 — Synchronize initial driving prototype with main

- Timestamp: 2026-07-17 18:31 (Europe/Warsaw)
- User request: Synchronize the open initial driving prototype branch with the current `main` branch.
- Baseline branch and commit: `agent/initial-driving-prototype` at `0e72e9fab10267a9130af03a65d818f927764886`; `main` at `8aef9c6c0f59b4f9c70c90c09c9c634e9ee4a189`.
- Repository history reviewed:
  - Re-read the complete current `AGENTS.md` and all prior entries in `docs/manager-history.md`.
  - Reviewed the complete visible `main` history through the private-runner-service merge, the complete current PR #1 final state and changed-file set, the current merge base, and the successful pre-synchronization CI evidence.
  - Reviewed helper PR #9, which targeted `agent/initial-driving-prototype` from `main` and represented the eight commits missing from the feature branch.
- Repository state found:
  - Before synchronization, PR #1 was 45 commits ahead and 8 commits behind `main`, with merge base `07664bb53fc0bf999796b924d98e9b6fd3fd0439`.
  - The prototype had already passed Repository validation run 43, Vehicle Simulation CI run 24, and Full Unreal Engine CI run 9, including a successful `NextcarEditor` build and 5/5 Unreal Automation Tests.
  - The GitHub connector does not expose a direct update-branch mutation, but permits a normal same-repository pull request whose base is the feature branch.
- Workstream decomposition and programmer-agent assignments:
  - The task was decomposed into integration-state verification, conflict-safe synchronization, post-merge comparison, full CI verification, and persistent manager logging.
  - No programmer agent was dispatched because synchronization is one atomic branch-integration operation and the shared branch/history files require sequential ownership.
- Files and behavior changed:
  - Opened helper PR #9 from `main` to `agent/initial-driving-prototype` and merged it with a normal merge commit `10111c14db1f482fb482cc0d158c216f91bfba88`, preserving both histories without rebasing or force-pushing.
  - The feature branch now contains the current repository policy, manager history, Windows runner runbook, and all prior prototype code and CI fixes.
  - Appended this manager-history entry. No gameplay behavior was changed by the synchronization itself.
- Evidence and exact tests:
  - Post-merge comparison reports the feature branch `ahead_by: 46`, `behind_by: 0`, with `main` commit `8aef9c6c0f59b4f9c70c90c09c9c634e9ee4a189` as the merge base.
  - GitHub Actions on synchronization commit `10111c14db1f482fb482cc0d158c216f91bfba88`: Repository validation run 44 — success; Vehicle Simulation CI run 25 — success; Full Unreal Engine CI run 10 — success.
  - Full Unreal Engine CI completed environment resolution, checkout verification, `NextcarEditor` compilation, all `Nextcar.*` Unreal Automation Tests, report validation, and artifact upload successfully.
- Decisions and integration notes:
  - Used a two-parent merge commit rather than rebase or force-push to preserve the reviewed prototype history and the current `main` history.
  - PR #1 remains open and draft; synchronization alone does not authorize merging the feature into `main`.
- Unresolved risks or blockers:
  - A manual in-editor driving smoke test has not yet been recorded after synchronization.
  - The Windows runner is currently being operated interactively as administrator rather than through the documented dedicated service account; service-mode verification remains outstanding.
- Next steps:
  - Confirm the final history-only commit passes Repository validation.
  - Perform and record a short manual driving smoke test before changing PR #1 from draft or merging it.

## 2026-07-17 — Correct wheel visual rotation axis

- Timestamp: 2026-07-17 18:49 (Europe/Warsaw)
- User request: Record the manual smoke-test result that the prototype works except that the wheels rotate around the wrong axis, then correct the defect.
- Baseline branch and commit: `agent/initial-driving-prototype` at `678b9aec20e66e7ac1454d0c768d55be794e69c7`.
- Repository history reviewed:
  - Re-read the complete current `AGENTS.md` and all prior entries in `docs/manager-history.md`.
  - Reviewed the current PR #1 branch state, the synchronized merge base, the wheel construction and visual-update implementation, the portable wheel-rotation state calculation, the existing Unreal smoke tests, and the successful CI evidence from the synchronized head.
- Repository state found:
  - The user manually confirmed that the current prototype launches and otherwise behaves correctly in the editor.
  - The engine cylinder mesh uses local `Z` as its symmetry axis, while `ConfigureWheel` used `Pitch=90` and therefore aligned the wheel axle with the car longitudinal `X` axis; `UpdateWheelVisuals` then applied rolling through `Roll`, preserving the incorrect axle.
  - PR #1 remained synchronized with `main` and open as a draft.
- Workstream decomposition and programmer-agent assignments:
  - The task was decomposed into geometric root-cause verification, a shared wheel-rotation transform, production integration, an Unreal regression test, full CI verification, and manual revalidation.
  - No programmer agent was dispatched because the correction is one small atomic code-and-test scope with shared transform ownership and required sequential validation on the single self-hosted Unreal runner.
- Files and behavior changed:
  - Added `Source/Nextcar/ArcadeWheelVisuals.h` with one canonical wheel transform: `Roll=90` aligns the cylinder axle across the car, `Pitch` applies rolling around that axle, and `Yaw` applies front-wheel steering.
  - Updated `Source/Nextcar/ArcadeCarPawn.cpp` so initial wheel orientation and every visual update use the shared transform.
  - Added `Nextcar.Smoke.WheelVisualAxes` in `Source/Nextcar/Tests/NextcarSmokeTests.cpp`; it verifies lateral axle alignment, axle preservation during rolling, radial movement around the axle, and steering around the vertical axis.
  - Appended this manager-history entry.
- Evidence and exact tests:
  - User manual smoke test on the previous synchronized build: launching and general driving behavior passed; wheel visual rotation axis failed.
  - GitHub Actions on code/test commit `98ad76bc1f36e9a9dc4cc3f6706bf7a692c7c10f`: Repository validation run 48 — success; Vehicle Simulation CI run 30 — success; Full Unreal Engine CI run 14 — success.
  - Full Unreal Engine CI successfully compiled `NextcarEditor`, ran all `Nextcar.*` tests including `Nextcar.Smoke.WheelVisualAxes`, validated the automation report, and uploaded logs and reports.
- Decisions and integration notes:
  - Kept wheel geometry and simulation state unchanged; only the mapping from simulated spin/steering angles to mesh orientation was corrected.
  - Centralized the transform to prevent constructor and per-frame visual logic from drifting apart.
  - PR #1 remains draft until the corrected wheel motion is manually confirmed in the editor.
- Unresolved risks or blockers:
  - The corrected visual result still requires a short manual editor test on the new head.
  - The Windows runner is still operated interactively as administrator rather than through the documented dedicated service account.
- Next steps:
  - Refresh the runner workspace or local test copy to the latest PR #1 head and verify forward, reverse, steering, and reset wheel animation in Play mode.
  - After manual confirmation, update the PR evidence/checklist and decide whether PR #1 is ready to leave draft and merge.
