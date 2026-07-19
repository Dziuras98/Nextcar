# NC-003B Phase 0 reconstruction audit

Status: **working audit — reconstruction in progress**

This document records the manager-owned reconstruction boundary for draft PR #23. It is not a Phase 0 approval record and it does not authorize Phase 1.

## Authoritative inputs

Only the staged source trees below are authoritative for upstream content during reconstruction:

- `Tools/EngineSimVendor/SourceInputs/NC-003B-source-inputs/engine-sim`
- `Tools/EngineSimVendor/SourceInputs/NC-003B-source-inputs/simple-2d-constraint-solver`

Pinned provenance:

- engine-sim repository: `Dziuras98/engine-sim`
- engine-sim commit: `85f7c3b959a908ed5232ede4f1a4ac7eafe6b630`
- engine-sim tree: `0756dea0444ada160540685dd1d28fcd3ef4aac5`
- simple-solver repository: `ange-yaghi/simple-2d-constraint-solver`
- simple-solver commit: `e009f4ff1c9c4c5874e865e893cdb62e208fb2b3`
- simple-solver tree: `20b64856226739dd46f0f6ed748e656702f0bb27`
- exact impulse-response Git blob: `6d3f8688e170cb6e5f4bfec42f580f3900514d72`

No cross-repository operation is permitted. The staged trees are temporary and must be removed before manager review.

## Closure roots

The Phase 0 dependency closure is derived from exactly these functional roots:

1. `SubaruEJ25AtgVideo2Fixture`, including all data needed to transcribe the pinned EJ25 script faithfully;
2. `PistonEngineSimulator`, reduced to the headless engine, solver and audio path required by the fixture-calibration harness;
3. the deterministic offline impulse-response and calibration-profile generators and their independent verifiers.

Renderer, UI, scripting runtime, vehicle, transmission, gameplay and dynamometer code are excluded unless a concrete compile-time dependency proves unavoidable. Any such exception must be documented before inclusion.

## File classification

### Exact-upstream files

Files copied byte-for-byte from one of the two staged source trees. Each final entry must record the upstream path, pinned source, SHA-256 and final SHA-256. For exact files both hashes must be identical.

### Patched-upstream files

Only files required by the closure and changed for one or more documented reasons:

- portability includes;
- C++17 ODR fixes;
- solver ownership cleanup;
- pointer-array zeroing;
- cross-platform include fixes;
- explicit ring-buffer occupancy;
- synchronous synthesizer support;
- per-instance deterministic RNG;
- removal of mutable function-static audio state;
- headless simulator reduction;
- exact harmonic cam-lobe generation.

Every patched file must have source path, upstream SHA-256, final SHA-256 and an exact rationale in `PATCHES.md` and `SOURCE_MANIFEST.json`.

### Generated files

Deterministic outputs recreated from committed inputs:

- `MinimalMuffling02ImpulseResponse.generated.h`;
- the selected cold-start profile header;
- committed calibration traces and evidence derived by the documented profile tooling.

Every generated output must be reproducible and independently verified against its committed source representation.

### Project-authored files

Fixture code, field-by-field parity verifier, synchronous calibration harness, generators, independent verifiers, tests, build integration, manifests, notices and update documentation. Project-authored code must not read runtime audio data from the filesystem and must not perform network, Git, GitHub, credential, release or PowerShell side effects.

### Files to remove before review

- the complete `Tools/EngineSimVendor/SourceInputs` directory;
- `Tools/EngineSimVendor/WIP_CHECKPOINT.md` unless replaced by accurate non-stale final documentation;
- any `.transport` or `.upload` directory;
- `RUNNER_BOOTSTRAP_REQUEST` and `bootstrap_on_runner.ps1`;
- base64 fragments, archive fragments, credential probes and release-transport residue;
- staging-only `.gitattributes`, `README_STAGE.md`, `ARCHIVE_SHA256.txt`, `SOURCE_INPUTS_MANIFEST.txt` and `SOURCE_INPUTS_SHA256.txt`.

## Confirmed defects in the accepted WIP

- the EJ25 exhaust collector area is incorrectly forced to `units::area(1.25 * 1.25, units::inch * units::inch)` although the pinned script relies on the library default from `circle_area(2.0 * units.inch)`;
- `Simulator::endFrame()` does not yet enforce an explicit synchronous production cap;
- the final engine-sim and solver closure, generated IR data, calibrated profile, evidence and complete provenance are not present;
- `WIP_CHECKPOINT.md` is stale because it still describes the exact WAV as unavailable;
- final runtime/build/tests must not depend on reading the WAV from the filesystem.

## Required synchronous cap

The initial production cap is at most `2000` frames. Direct tests must cover zero pending frames, a partial buffer, exactly 2000 pending frames and more than 2000 pending frames. The return value must equal the number of frames actually appended to the native ring buffer.

## Current phase state

```text
Phase 0 WIP — reconstruction in progress, not submitted for manager approval
Phase 1 not started
```
