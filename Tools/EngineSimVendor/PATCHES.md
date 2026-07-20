# NC-003B Phase 0 working patch ledger

> Status: Phase 0 WIP — fixture parity implementation published, exact-head C++ validation pending, calibration not started. Not submitted for manager approval. Phase 1 not started.

## Source authority

- Repository: `Dziuras98/engine-sim`
- Commit: `85f7c3b959a908ed5232ede4f1a4ac7eafe6b630`
- Tree: `0756dea0444ada160540685dd1d28fcd3ef4aac5`
- Authoritative local input: `Tools/EngineSimVendor/SourceInputs/NC-003B-source-inputs/engine-sim`

No cross-repository fetch is required or permitted for this reconstruction.

## Translation-unit classification

- Exact upstream translation units: **21**
- Patched upstream translation units: **12**
- Total translation units: **33**

Exact-upstream destinations remain byte-identical to their SourceInputs counterparts. `SOURCE_MANIFEST.json` retains source/final paths and SHA-256 values.

## Patched upstream files

- `src/camshaft.cpp`: explicit portability include.
- `src/combustion_chamber.cpp`: accepted parameter names and per-instance xorshift32 seeded by `Parameters::randomSeed`.
- `src/connecting_rod.cpp`: portable include spelling and owned journal cleanup.
- `src/cylinder_head.cpp`: accepted lower-case parameter names.
- `src/engine.cpp`: Engine-only headless simulator creation and ownership.
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

The corrected fixture evidence includes `display_depth = 0.5` rather than `0.4`, a separate 30-point chamber turbulence function, explicit fuel defaults including `max_dilution_effect = 10`, `pos_x = 0`, `pos_y = 0`, deck height `195.5068 mm`, circular two-inch collector area, deterministic seeds `0x4E433033..0x4E433036`, and exclusion of Vehicle, Transmission and runtime WAV I/O. Earlier calibration attempts remain non-evidence; calibration has not started.

See `Tools/EngineSimVendor/FIXTURE_TRANSCRIPTION.md` for the full evidence record.

## Validation status

Python `py_compile`, all 21 verifier unit tests, all 118 field records, every shard SHA-256 and the semantic contract digest passed locally. Exact-head C++ validation is pending because the execution environment did not contain the complete checkout/closure. No C++ PASS is claimed.

## Scope note

The simple-solver closure, SourceInputs, exact WAV paths, generated IR, cold-start profiles, benchmarks, gameplay, Runtime Audio and Phase 1 API are unchanged. PR #23 remains draft; Phase 0 is not submitted for manager approval.
