# Subaru EJ25 fixture-transcription parity

> Status: Phase 0 WIP — fixture parity Windows exact-head validated and cold-start calibration published. Manager review pending; not submitted for manager approval. Phase 1 not started.

## Pinned source

- Source repository: `Dziuras98/engine-sim`
- Pinned engine-sim commit: `85f7c3b959a908ed5232ede4f1a4ac7eafe6b630`
- Pinned engine-sim tree: `0756dea0444ada160540685dd1d28fcd3ef4aac5`
- Source script: `assets/engines/atg-video-2/01_subaru_ej25_eh.mr`
- Source script Git blob: `586ad92f1f27d097d5c422e2484915acc7c6a5d2`
- Source script SHA-256: `04eba9d5e0f8fa2d1b2d6bfc89ee09cd1a414ecbd81d2b39f6cfd6c04fcdcb5a`

The source authority remains the preserved local `Tools/EngineSimVendor/SourceInputs/NC-003B-source-inputs/engine-sim` tree. No cross-repository fetch is required for verification.

## Canonical contract

`Tests/EngineSimCore/Fixtures/subaru_ej25_atg_video_2_fixture.json` is a small index. It names an ordered set of standalone semantic JSON shards under `Tests/EngineSimCore/Fixtures/subaru_ej25_atg_video_2_fixture/`.

The combined contract contains exactly **118 records**. Each field ID occurs once. The canonical digest is computed from the ordered combined record array using `json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=False)` encoded as UTF-8.

- Semantic contract SHA-256: `3ba447789024d613cdceb2382d917e9d6ce340cbeecb621d4f71133e55578f00`
- Source monolithic candidate SHA-256: `096c4e2ed9b310bc1d03cffd44090ba0515162ba56ca0594ffaefe7a72bc0eaf`
- Category counts: explicit-script 67; resolved-library-default 17; derived-from-script 10; headless-construction 8; determinism-patch 1; generated-asset 7; excluded-non-headless 8.

The monolithic hash records provenance only. It is not the hash of the index. Record-by-record comparison confirmed full object equality between the original 118-record candidate and the ordered shard reconstruction.

## Corrected transcription evidence

The fixture parity candidate corrects all discrepancies identified by the field-by-field audit:

- bank `display_depth` is `0.5`, correcting the earlier `0.4` value;
- combustion chambers use a separate 30-point mean-piston-speed turbulence function rather than the fuel turbulence function;
- omitted fuel library defaults are explicit in the snapshot, including `max_dilution_effect = 10`;
- `Crankshaft::Parameters::pos_x` and `pos_y` are populated explicitly with zero;
- deck height follows `stroke / 2 + rod length + compression height` and resolves to `195.5068 mm`;
- exhaust collector area preserves `circle_area(2.0 * units.inch)` semantics, resolving to π square inches;
- deterministic per-cylinder seeds remain `0x4E433033` through `0x4E433036`;
- generated impulse response metadata remains 17,555 samples, 44,100 Hz, mono PCM16, with fixture gain `0.01`;
- the headless construction has no `Vehicle` or `Transmission` dependency and performs no runtime WAV I/O.

Earlier cold-start and profile calibration attempts are non-evidence for this parity gate. Calibration has not started in the reconstructed workspace.

## Verification

`Tools/EngineSimVendor/verify_subaru_ej25_fixture.py` validates index metadata, safe relative shard paths, ordered shard names, per-shard SHA-256, per-shard and total record counts, unique field IDs, category counts, the semantic digest, pinned source script hash/blob, source anchors, fixture snapshot values, generated IR provenance, headless exclusions and the existing deterministic fixture constraints.

`Tools/EngineSimVendor/tests/test_subaru_ej25_fixture_verifier.py` retains the original 15 mutation tests and adds rejection tests for a missing shard, modified shard bytes, duplicate records across shards, changed ordering, semantic digest mismatch and path traversal.

Python validation passed on the exact Windows checkout: `py_compile`, 21/21 unit tests, all shard hashes, the semantic contract digest and all 118 field records.

## Windows exact-head standalone validation

- Validated source SHA: `542d3261efc3ef48c78f337d990793aea55dd7fb`
- Workflow commit SHA: `fccf4b83ffa9f73b7bf99c2f45fa87277d9d267a`
- GitHub Actions run ID: `29756153748`
- Final run attempt: `2`
- Runner image: `windows-2022` (`20260714.244.1`)
- Operating system: Windows Server 2022 x64
- Visual Studio: `17.14.37411.7`
- MSVC: `19.44.35228`
- VCTools: `14.44.35207`
- MSBuild: `17.14.40.60911`
- Windows SDK: `10.0.26100.0`
- Python: `3.12.10`
- CMake: `3.31.6`
- Python result: `py_compile` PASS; unit tests 21/21 PASS; shard hashes PASS; semantic digest PASS; records 118/118 PASS.
- MSVC x64 Release: configure, build, CTest and runtime PASS.
- MSVC x64 Debug `/RTC1`: configure, build, CTest and runtime PASS.
- MSVC x64 AddressSanitizer: configure, build, CTest and runtime PASS.
- Clean reproducibility: Python, Release build, CTest and runtime PASS; stdout byte-for-byte identical.
- Project warning count: `0`.
- Overall result: `Windows exact-head standalone validation PASS`.
- Windows MSVC Release, Debug `/RTC1` and AddressSanitizer did not reproduce the sparse-matrix zero-size UB.

Expected and observed standalone executable stdout:

```text
PASS ring-buffer
PASS synchronous-synthesizer
PASS deterministic-rng
PASS solver-runtime
PASS impulse-response-integrity
PASS fixture-transcription-parity
PASS fixture-simulator-smoke
```

Attempt 1 completed the full Windows matrix successfully and intentionally failed only the notification sentinel. Failed-job-only rerun attempt 2 completed the sentinel and retained the matrix PASS.

## Supplementary non-gating Linux result

- GCC Release: PASS.
- Clang Release: PASS.
- Linux UBSan found a zero-size `memset(nullptr, ..., 0)` UB in the pinned `simple-2d-constraint-solver` sparse-matrix path.
- The Linux sanitizer finding is supplementary and is not a Windows acceptance gate.
- The solver was not changed because Windows MSVC Release, Debug `/RTC1` and AddressSanitizer did not reproduce the failure.

## Scope boundaries

This work does not modify the generated IR, IR generator, IR verifier, exact WAV files, simple-solver, SourceInputs, cold-start profiles, benchmarks, gameplay, Runtime Audio or the public Phase 1 API. PR #23 remains draft. Phase 0 is not submitted for manager approval.

## Cold-start calibration linkage

The field-by-field fixture-transcription contract, its 118 records, semantic SHA-256 `3ba447789024d613cdceb2382d917e9d6ce340cbeecb621d4f71133e55578f00`, mechanical parameters, deterministic seeds, IR provenance and Windows exact-head parity evidence remain unchanged.

The separately measured Phase 0 cold-start profile is `SubaruEJ25AtgVideo2ColdStartV1`. It uses startup throttle `0.05`, starter disengagement at `800 RPM`, a required post-starter minimum of `600 RPM`, a `2.0 s` stability window and an `8.0 s` maximum simulation window. Windows run `29814599329`, final attempt 2, produced 10/10 successful deterministic trials plus passing timeout and stall-after-disengagement cases. Full trace hashes, distributions, margins, generated-input hashes and toolchain evidence are recorded in `Tools/EngineSimVendor/COLD_START_CALIBRATION.md`.

No fixture-transcription shard, source-script provenance, exact WAV, generated IR, engine-sim pin, solver pin or excluded non-headless dependency was changed by calibration.
