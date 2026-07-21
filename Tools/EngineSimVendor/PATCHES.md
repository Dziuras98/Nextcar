# NC-003B Phase 0 working patch ledger

> Status: Phase 0 WIP — final closure candidate prepared without temporary SourceInputs; Windows exact-head closure validation pending. Phase 1 not started.

## Source authority

- Repository: `Dziuras98/engine-sim`
- Commit: `85f7c3b959a908ed5232ede4f1a4ac7eafe6b630`
- Tree: `0756dea0444ada160540685dd1d28fcd3ef4aac5`
- Active fixture-source evidence: `Tools/EngineSimVendor/FixtureSourceSnapshot/engine-sim`
- Snapshot index: `Tools/EngineSimVendor/FixtureSourceSnapshot/SNAPSHOT.json`

Normal build and verification require neither cross-repository access nor the removed staging tree. The four-file snapshot is the active source evidence for field-by-field fixture parity.

## Translation-unit classification

- Exact upstream translation units: **20**
- Patched upstream translation units: **13**
- Total translation units: **33**

Exact-upstream destinations remain byte-identical to the pinned inputs. Historical `source_path` values in `SOURCE_MANIFEST.json` are provenance only; active verification uses the fixture-source snapshot.

## Patched upstream files

- `src/camshaft.cpp`: explicit portability include.
- `src/combustion_chamber.cpp`: accepted parameter names and per-instance xorshift32 seeded by `Parameters::randomSeed`.
- `src/connecting_rod.cpp`: portable include spelling and owned journal cleanup.
- `src/cylinder_head.cpp`: accepted lower-case parameter names.
- `src/engine.cpp`: Engine-only headless simulator creation and ownership.
- `src/gaussian_filter.cpp`: explicit standard-algorithm include required by MSVC v143 for `std::max`.
- `src/ignition_module.cpp`: explicit assertion include.
- `src/impulse_response.cpp`: in-memory PCM contract; no runtime WAV loading.
- `src/jitter_filter.cpp`: explicit C string support and owned buffer cleanup.
- `src/piston.cpp`: accepted lower-case parameter names.
- `src/piston_engine_simulator.cpp`: no Vehicle, Transmission, Dynamometer or drag integration; synchronous audio path.
- `src/simulator.cpp`: Engine-only load, minimal solver ownership, no worker lifecycle, synchronous production cap and in-memory convolution initialization.
- `src/synthesizer.cpp`: no thread/mutex/condition-variable worker path; bounded synchronous processing and deterministic per-instance noise RNG.
- `include/combustion_chamber.h`: stable `Parameters::randomSeed` default.
- `include/filter.h`: non-MSVC `__forceinline` compatibility.
- `include/ring_buffer.h`: explicit occupancy, distinct full/empty state and checked bounds.

## Deterministic Subaru impulse response

The exact WAV, generated header, generator and independent IR verifier are unchanged by the fixture-parity publication. The source Git blob remains `6d3f8688e170cb6e5f4bfec42f580f3900514d72`; WAV SHA-256 remains `df0b8be829d28ae64e5b2818a1942a3b3e5733186bdf262cad4c2af038995d48`; PCM SHA-256 remains `ce0702aa501d94f35b5f804efcd1b21688b9f9cacaa0fa2fc7f459c03a98e540`. The fixture uses all 17,555 mono PCM16 samples at 44,100 Hz with gain `0.01` and performs no runtime WAV I/O.

## Subaru fixture-transcription parity gate

The canonical field contract is now a small index plus ordered semantic shards. It contains exactly 118 unique records and has semantic SHA-256 `3ba447789024d613cdceb2382d917e9d6ce340cbeecb621d4f71133e55578f00`. The source monolithic candidate SHA-256 `096c4e2ed9b310bc1d03cffd44090ba0515162ba56ca0594ffaefe7a72bc0eaf` is retained only as provenance.

The verifier rejects unsafe shard paths, missing files, ordering changes, shard hash/count changes, duplicate IDs, category-count changes and semantic digest changes before running the pre-existing source, fixture, snapshot and IR checks.

The corrected fixture evidence includes `display_depth = 0.5` rather than `0.4`, a separate 30-point chamber turbulence function, explicit fuel defaults including `max_dilution_effect = 10`, `pos_x = 0`, `pos_y = 0`, deck height `195.5068 mm`, circular two-inch collector area, deterministic seeds `0x4E433033..0x4E433036`, and exclusion of Vehicle, Transmission and runtime WAV I/O. Earlier unsuccessful calibration attempts remain non-evidence. The accepted cold-start calibration is recorded separately in `Tools/EngineSimVendor/COLD_START_CALIBRATION.md`.

See `Tools/EngineSimVendor/FIXTURE_TRANSCRIPTION.md` for the full evidence record.

## Subaru cold-start calibration

The Phase 0 calibration harness evaluates deterministic `1/120 s` simulation slices on fresh fixture instances, records only actually produced native and PCM frames, rejects non-finite values, measures RMS from produced samples, and always disables starter and ignition before destroying simulator and fixture state.

The selected candidate remains `thr-0p050-dis-0800-min-0600-win-2p000-max-8p000`: startup throttle `0.05`, starter disengagement at `800 RPM`, post-starter minimum `600 RPM`, stability window `2.0 s`, and maximum startup simulation time `8.0 s`. Windows run `29814599329`, final attempt 2, passed 10/10 fresh trials, deterministic timeout and stall-after-disengagement cases, profile generation, header generation, strict verification, mutation tests, MSVC x64 Release, Debug `/RTC1`, AddressSanitizer, CTest, direct runtime, clean reproducibility, and project warning count 0.

The measured profile SHA-256 is `2e8a6795c85e648606e174a00cc96230363b6bc60a78cd62374490377414bb1d`; the generated header SHA-256 is `8d44a17e4491dfdb1500ff8498c739c143fffa84510df2f31bfdf9a0e3797d5f`. The profile does not introduce runtime WAV loading and does not modify fixture mechanics, engine-sim provenance, solver provenance, the exact impulse response, or fixture-transcription parity.

## Validation status

Windows exact-head validation passed for source SHA `542d3261efc3ef48c78f337d990793aea55dd7fb` in GitHub Actions run `29756153748`, final attempt 2, on `windows-2022` with Visual Studio 2022 and MSVC v143. Python `py_compile`, all 21 verifier unit tests, all 118 field records, every shard SHA-256 and the semantic contract digest passed. MSVC x64 Release, Debug `/RTC1`, AddressSanitizer, CTest, runtime and clean reproducibility passed with identical stdout and project warning count 0. Windows did not reproduce the sparse-matrix zero-size UB. The GCC and Clang Release PASS results remain supplementary; the Linux UBSan solver finding is non-gating and the pinned solver is unchanged.

## Scope note

The temporary `Tools/EngineSimVendor/SourceInputs` tree was removed after its four required fixture-source files were preserved byte-for-byte in the indexed snapshot. The exact WAV, generated IR, solver, benchmarks, gameplay, Runtime Audio and Phase 1 API are unchanged. Final Windows exact-head closure validation remains pending; PR #23 remains draft.
