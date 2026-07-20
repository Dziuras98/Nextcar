# NC-003B Phase 0 working patch ledger

> Status: Phase 0 WIP — reconstruction in progress, not submitted for manager approval. Phase 1 not started.

## Source authority

- Repository: `Dziuras98/engine-sim`
- Commit: `85f7c3b959a908ed5232ede4f1a4ac7eafe6b630`
- Tree: `0756dea0444ada160540685dd1d28fcd3ef4aac5`
- Authoritative local input: `Tools/EngineSimVendor/SourceInputs/NC-003B-source-inputs/engine-sim`

No cross-repository fetch is required or permitted for this reconstruction.

## Translation-unit classification

- Exact upstream: **21**
- Patched upstream: **12**
- Total: **33**

Every exact-upstream destination is byte-identical to its local SourceInputs counterpart. `SOURCE_MANIFEST.json` records every source/final path and SHA-256.

## Patched upstream files

### `Plugins/NextcarEngineSim/Source/ThirdParty/EngineSim/upstream/engine-sim/src/camshaft.cpp`

- Source path: `Tools/EngineSimVendor/SourceInputs/NC-003B-source-inputs/engine-sim/src/camshaft.cpp`
- Upstream SHA-256: `1748dba81f82e70d1a894705a686394711753de98e213c5e462d0709ce7d40bd`
- Final SHA-256: `80e642d42568b646391fe336f5b086c07cbb42223a56962358779256c01fa2d7`
- Reason: portability includes.
- Concrete changes: Add the explicit C string include required by memset.

### `Plugins/NextcarEngineSim/Source/ThirdParty/EngineSim/upstream/engine-sim/src/combustion_chamber.cpp`

- Source path: `Tools/EngineSimVendor/SourceInputs/NC-003B-source-inputs/engine-sim/src/combustion_chamber.cpp`
- Upstream SHA-256: `b4fa30a952bf7fcb8d75a3be51e93bdff077c16588794d7721c8c361a04988f7`
- Final SHA-256: `cbed18f5e0818192ca37956ec8e0d3a13254aead70dba86e2699154361eff6c0`
- Reason: per-instance deterministic RNG.
- Concrete changes: Use accepted parameter names, seed from Parameters::randomSeed, and replace global rand() with per-instance xorshift32.

### `Plugins/NextcarEngineSim/Source/ThirdParty/EngineSim/upstream/engine-sim/src/connecting_rod.cpp`

- Source path: `Tools/EngineSimVendor/SourceInputs/NC-003B-source-inputs/engine-sim/src/connecting_rod.cpp`
- Upstream SHA-256: `e56e7fb0d1b362fbd4c89dd6cce7843082480855a3d8cb447b344d19f195e821`
- Final SHA-256: `e0222c757de0bd571b26cc479dc25436a478f8ff2bc97a89062ea9ee08fad05f`
- Reason: ownership and portability fixes.
- Concrete changes: Use portable include spelling and implement destroy() for owned journal storage and borrowed-pointer reset.

### `Plugins/NextcarEngineSim/Source/ThirdParty/EngineSim/upstream/engine-sim/src/cylinder_head.cpp`

- Source path: `Tools/EngineSimVendor/SourceInputs/NC-003B-source-inputs/engine-sim/src/cylinder_head.cpp`
- Upstream SHA-256: `70d3b184572ed3fb5503122e8ca0fe0eb564d579a30585582964491f9dd284cb`
- Final SHA-256: `2d83a9e2e11a454aec90d794adbec6cfe19df89f1b30c71153afca6e3fa9cd45`
- Reason: header/implementation consistency.
- Concrete changes: Map implementation access to the accepted lower-case Parameters names without changing calculations.

### `Plugins/NextcarEngineSim/Source/ThirdParty/EngineSim/upstream/engine-sim/src/engine.cpp`

- Source path: `Tools/EngineSimVendor/SourceInputs/NC-003B-source-inputs/engine-sim/src/engine.cpp`
- Upstream SHA-256: `b8a5d4c21bccd65ae684654ec280a45de93cef8d8f16b47b2ded6a2d7761bfbe`
- Final SHA-256: `6c8ad6ac779608ac30dd64cdeb0fc0cfa0aa3746eb9f28aec8a8e3d6db73bc13`
- Reason: headless simulator reduction and ownership.
- Concrete changes: Remove Vehicle/Transmission from createSimulator(), construct an Engine-only PistonEngineSimulator, and retain Phase 0 ownership/mechanics.

### `Plugins/NextcarEngineSim/Source/ThirdParty/EngineSim/upstream/engine-sim/src/ignition_module.cpp`

- Source path: `Tools/EngineSimVendor/SourceInputs/NC-003B-source-inputs/engine-sim/src/ignition_module.cpp`
- Upstream SHA-256: `b926af3b359fc622410c88611c9c135fd003ee4946e94f388b3f37307c5f463e`
- Final SHA-256: `941f884681321917efbaaa05fcc5d483d1c0eba04cad5cfc23eb2f39cf8bfd9e`
- Reason: portability includes.
- Concrete changes: Add the explicit assertion include required by the implementation.

### `Plugins/NextcarEngineSim/Source/ThirdParty/EngineSim/upstream/engine-sim/src/impulse_response.cpp`

- Source path: `Tools/EngineSimVendor/SourceInputs/NC-003B-source-inputs/engine-sim/src/impulse_response.cpp`
- Upstream SHA-256: `13442f10d94f8c6da088944b007f5dcb55d456d5ccd83e214c417b5a6c79097e`
- Final SHA-256: `7d91ff29a534fc1272b0ba3baf20c41d82ad8070606fb39fda13a7aeaba99c08`
- Reason: in-memory impulse-response data path.
- Concrete changes: Remove runtime WAV loading and use the in-memory PCM contract.

### `Plugins/NextcarEngineSim/Source/ThirdParty/EngineSim/upstream/engine-sim/src/jitter_filter.cpp`

- Source path: `Tools/EngineSimVendor/SourceInputs/NC-003B-source-inputs/engine-sim/src/jitter_filter.cpp`
- Upstream SHA-256: `b09a6b6e2406bc78161aba276ad551c3c2e4c8d1a3b5e8fc7d3cfa16aff1c439`
- Final SHA-256: `ad43a5a4b4234695bb94bec421fcfa6082294477542325b5a6c576d347e77f37`
- Reason: portability and ownership.
- Concrete changes: Add explicit C string support and implement destroy() for the owned sample buffer.

### `Plugins/NextcarEngineSim/Source/ThirdParty/EngineSim/upstream/engine-sim/src/piston.cpp`

- Source path: `Tools/EngineSimVendor/SourceInputs/NC-003B-source-inputs/engine-sim/src/piston.cpp`
- Upstream SHA-256: `a14fa6bbf682f7838414f5a5513773733b60b7fd4076c39e35de7bf95fe5af6b`
- Final SHA-256: `a8ceda89b692476fa0f8f9fb5bfb6a02aefbfd50cede56741df97fe631fdfaec`
- Reason: header/implementation consistency.
- Concrete changes: Map implementation access to the accepted lower-case Parameters names.

### `Plugins/NextcarEngineSim/Source/ThirdParty/EngineSim/upstream/engine-sim/src/piston_engine_simulator.cpp`

- Source path: `Tools/EngineSimVendor/SourceInputs/NC-003B-source-inputs/engine-sim/src/piston_engine_simulator.cpp`
- Upstream SHA-256: `0da7b8fd41e2da0074e19f1ed056e56f24c512c67ac828460355c8549eec38ad`
- Final SHA-256: `13e04758beaeab080fef5e101c2676ba07d66318905681877c7ffee4167de404`
- Reason: headless solver and synchronous audio.
- Concrete changes: Remove Vehicle, Transmission, Dynamometer and drag integration; use the mechanical solver/starter only; remove mutable function-static audio state; route endFrame() synchronously.

### `Plugins/NextcarEngineSim/Source/ThirdParty/EngineSim/upstream/engine-sim/src/simulator.cpp`

- Source path: `Tools/EngineSimVendor/SourceInputs/NC-003B-source-inputs/engine-sim/src/simulator.cpp`
- Upstream SHA-256: `3a014d5dd7e2584250db33605fc51d07d1f7779b811f92c8599d19a654831eb4`
- Final SHA-256: `dd8b9bcec486639e401f6dba774af56123189cba6393f1f676ca8950c447433e`
- Reason: headless synchronous simulator.
- Concrete changes: Load Engine only, own the minimal optimized NSV solver, remove worker lifecycle, cap synchronous production at 2000, and initialize convolution from in-memory PCM.

### `Plugins/NextcarEngineSim/Source/ThirdParty/EngineSim/upstream/engine-sim/src/synthesizer.cpp`

- Source path: `Tools/EngineSimVendor/SourceInputs/NC-003B-source-inputs/engine-sim/src/synthesizer.cpp`
- Upstream SHA-256: `8ec3421e98b32f811cb3fcc8453ed30de77fef1fe4fc6a2fcb69421175ae34e8`
- Final SHA-256: `d4b215fb7d4707f41b594fa19ca6c7e27ea5bd02c8894501550bc20fb05d2596`
- Reason: synchronous deterministic synthesizer.
- Concrete changes: Remove thread/mutex/condition-variable worker paths; implement processSynchronous(maxFrames); use explicit occupancy/capacity and per-instance deterministic noise RNG.

### `Plugins/NextcarEngineSim/Source/ThirdParty/EngineSim/upstream/engine-sim/include/combustion_chamber.h`

- Source path: `Tools/EngineSimVendor/SourceInputs/NC-003B-source-inputs/engine-sim/include/combustion_chamber.h`
- Upstream SHA-256: `218753e40a2b4995e398bce43d8b555a47431b0498830ea292af02a3a27e98b7`
- Final SHA-256: `ec42478bd985b9f96d1fdf3a086d10755256c20fd32c11b6357268855b727473`
- Reason: per-instance deterministic RNG.
- Concrete changes: Add Parameters::randomSeed with a stable default for per-instance deterministic RNG.

### `Plugins/NextcarEngineSim/Source/ThirdParty/EngineSim/upstream/engine-sim/include/filter.h`

- Source path: `Tools/EngineSimVendor/SourceInputs/NC-003B-source-inputs/engine-sim/include/filter.h`
- Upstream SHA-256: `e0a60775cdb57a5948e71298a33aca26bbd81973b4b28da7afabcc23c3f11c2f`
- Final SHA-256: `85185d465adf862d3ac3b3692e795912dd04dfed2f0e2362965581dafdf3e64e`
- Reason: cross-platform include compatibility.
- Concrete changes: Provide the non-MSVC __forceinline compatibility definition used by pinned filter headers.

### `Plugins/NextcarEngineSim/Source/ThirdParty/EngineSim/upstream/engine-sim/include/ring_buffer.h`

- Source path: `Tools/EngineSimVendor/SourceInputs/NC-003B-source-inputs/engine-sim/include/ring_buffer.h`
- Upstream SHA-256: `537345f0cb3d506ad72db3edd3bd4f78e03b8d4842c140f3e7fe5df66ef578c3`
- Final SHA-256: `e8ebea1e0657e5c959ccc40bf66d252f99862d5df93ad7c1151572fd89262e9b`
- Reason: explicit occupancy and safe bounds.
- Concrete changes: Track occupancy independently of indices; distinguish full from empty; reject writes, over-reads and over-removes safely.

## Project-authored support files

- `Plugins/NextcarEngineSim/Source/NextcarEngineSimCore/Private/Fixtures/SubaruEJ25AtgVideo2Fixture.cpp`: exact harmonic cam-lobe generation, stable per-cylinder seeds, `circle_area(2.0 * units.inch)` collector semantics, and initialization from the generated full Subaru EJ25 PCM with gain `0.01`; the one-sample identity placeholder was removed.
- `Tools/EngineSimVendor/Standalone/CMakeLists.txt`: explicit offline C++17 build graph with no Unreal, FetchContent, network, Git, credentials, scripting, UI or application target.
- `Tools/EngineSimVendor/Standalone/tests/phase0_standalone_tests.cpp`: direct runtime coverage for generated impulse-response metadata and full PCM accessibility, ring-buffer occupancy/bounds, the real synchronous Synthesizer cap, deterministic RNG, solver runtime, and fixture/simulator lifecycle.


## Deterministic Subaru impulse response

- Exact input: `Plugins/NextcarEngineSim/Source/ThirdParty/EngineSim/upstream/engine-sim/es/sound-library/new/minimal_muffling_02.wav`.
- Source Git blob: `6d3f8688e170cb6e5f4bfec42f580f3900514d72`.
- Generator: `Tools/EngineSimVendor/generate_impulse_response_header.py` uses a strict standard-library RIFF parser and writes atomically without timestamps or environment-dependent data.
- Generated output: `Plugins/NextcarEngineSim/Source/NextcarEngineSimCore/Private/Generated/MinimalMuffling02ImpulseResponse.generated.h` contains the complete decoded PCM16 sequence and stable metadata.
- Independent verification: `Tools/EngineSimVendor/verify_impulse_response.py` independently validates raw RIFF fields, Python `wave` decoding, every generated PCM value, hashes, manifest linkage, generation command, and notice mapping.
- Fixture integration: `Phase0PlaceholderImpulse` was removed; the fixture passes the generated PCM pointer, full generated sample count, generated sample rate, and gain `0.01` to the in-memory `ImpulseResponse` contract.
- Runtime I/O: ordinary Core construction, standalone tests, simulator and synthesizer do not open or parse the WAV and do not depend on the working directory.

## Headless exclusions

The closure does not reference `Vehicle`, `Transmission`, `Dynamometer`, `VehicleDragConstraint`, renderer/UI, scripting runtime, `std::thread`, `std::condition_variable`, `waitProcessed`, `startAudioRenderingThread`, `endAudioRenderingThread`, `audioRenderingThread`, or filesystem WAV loading.

## Validation status

Test results are not stored here as final evidence. Commands, compiler versions, exit codes and output from the exact published head are reported separately after commits 1–3 are published. Clang ASan is not required because the container runtime fails before `main()` for an independent hello-world executable; GCC ASan plus UBSan is the sanitizer authority for this checkpoint.

## Scope note

This checkpoint does not modify the already-published simple-solver closure. `SourceInputs` and both exact WAV paths remain preserved. Calibration, profiles, evidence, Unreal validation and Phase 1 remain outside this checkpoint.
