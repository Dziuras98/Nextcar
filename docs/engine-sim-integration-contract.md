# Engine-sim integration contract

Status: **Architecture frozen; NC-003A is ready for review and merge after repository validation**

Contract version: **1.3**

Target project: **nEXTcAR vertical slice**

This document is the architecture, ownership, provenance, calibration, and acceptance contract for the first `engine-sim` integration spike.

Version 1.3 preserves the accepted version 1.1 and 1.2 decisions for automatic Core-managed cold start, synchronous owner-thread simulation and synthesis, lifecycle behavior, native-ring initialization, `Advance()`/`PullPcm()` ordering, actual-produced PCM accounting, the exact simple-solver pin, and NC-003B Phase 0/Phase 1. It restores the pinned Subaru EJ25 impulse response as the mandatory production fixture, freezes its complete provenance, and requires deterministic offline conversion to a generated C++ header without runtime WAV I/O.

The architectural decisions are frozen. After PR #11 is merged and the manager creates the shared plugin scaffold, NC-003B, NC-003C, and NC-003D may begin in parallel within the non-overlapping write scopes in section 10. The cold-start values remain mandatory and may not be guessed. Their absence or unsupported calibration blocks acceptance and merge of NC-003B, not NC-003A.

## 1. Baseline and primary evidence

### 1.1 Repository baselines

| Repository | Branch | Pinned commit | Use in this contract |
|---|---|---|---|
| `Dziuras98/Nextcar` | `main` | `3ce2579f45b568e2c5fd43ee26249b561a055f1c` | Project structure, gameplay boundary, tests, workflows, Unreal version, and integration policy. |
| `Dziuras98/engine-sim` | `master` | `85f7c3b959a908ed5232ede4f1a4ac7eafe6b630` | Core source graph, simulator and synthesizer behavior, fixed mechanical/audio fixture, dependency graph, and root license. |
| `ange-yaghi/engine-sim` | upstream repository | introduction commit `4f7e06b211d0b51914aed0539b397ac27f70d0f3` | Original provenance for the mandatory Subaru EJ25 impulse-response WAV. |
| `ange-yaghi/simple-2d-constraint-solver` | gitlink dependency | `e009f4ff1c9c4c5874e865e893cdb62e208fb2b3` | Required solver source used by `PistonEngineSimulator`. |

The target engine version is **Unreal Engine 5.8**. `Nextcar.uproject` at the pinned Nextcar commit declares `EngineAssociation: "5.8"`.

### 1.2 Nextcar evidence reviewed

The contract is based on the complete project policy and integration boundary, including:

- `AGENTS.md`;
- `docs/manager-history.md`;
- `README.md`;
- `Nextcar.uproject`;
- `Source/Nextcar/Nextcar.Build.cs`;
- `Source/Nextcar/ArcadeVehicleSimulation.h`;
- repository and Unreal workflows;
- standalone and Unreal Automation Tests;
- `scripts/validate_repository.py`;
- PR #11 and manager comments, including `#issuecomment-5006829488`;
- the final user-approved NC-003A version 1.3 provenance decision.

Verified project facts:

1. The existing gameplay module has no engine-sim or procedural-audio dependency.
2. The existing arcade vehicle state is not an engine, clutch, gearbox, or driveline model.
3. The current gameplay implementation is an integration boundary to preserve.
4. The first engine-sim slice must remain independently testable and must not require gameplay changes.

### 1.3 engine-sim evidence reviewed

The selected closure was reviewed at engine-sim commit `85f7c3b959a908ed5232ede4f1a4ac7eafe6b630`, including:

- root `LICENSE`, `.gitmodules`, and CMake dependency declarations;
- simulator, piston-engine simulator, synthesizer, convolution/filter, ring buffer, engine, ignition, starter, throttle, dynamometer, transmission, and vehicle sources;
- `src/engine_sim_application.cpp`;
- `assets/main.mr`;
- `assets/engines/atg-video-2/01_subaru_ej25_eh.mr`;
- `es/sound-library/new/minimal_muffling_02.wav`;
- the introduction commit `4f7e06b211d0b51914aed0539b397ac27f70d0f3` and its added engine configurations and impulse-response files;
- wrapper headers for the simple solver, delta, and csv-io.

Frozen facts:

1. The root project contains a separable C++17 `engine-sim` target; the original application, UI, rendering, scripting, Discord, and direct-to-video code are not required by the Core adapter.
2. Native output is signed int16 mono at 44,100 Hz.
3. `readAudioOutput` returns the count actually consumed from the native ring and zero-fills only the caller-visible shortage tail.
4. The pinned ring has no full/empty discriminator. The upstream initialization loop that writes exactly the 44,100-sample capacity wraps its indices to equality, so the ring is logically empty and reports zero available samples.
5. There is no readable 44,100-frame startup prefix and no fixed startup drain.
6. Ignition and starter both initialize disabled and are controlled independently in the original application.
7. `Simulator::endFrame()` provides the frozen point after which one bounded synchronous synthesizer pass runs.
8. The upstream private audio-renderer thread is not part of the Nextcar architecture.
9. `PistonEngineSimulator` requires the simple 2D constraint solver.
10. `delta-studio`/`delta-basic` and `csv-io` are not part of the intended minimal source closure.
11. The mechanical Subaru EJ25 fixture requests 20,000 Hz simulation and specifies a 70 lb-ft, 500 RPM starter, but those mechanical values do not determine safe cold-start thresholds.
12. The Subaru EJ25 fixture's mandatory production impulse response is the exact WAV blob described in section 1.5; it is not replaceable by the identity unit-test fixture.

### 1.4 Exact solver provenance

The simple-solver pin is resolved as follows:

- engine-sim commit that changed the gitlink:
  `b92176f911463f143f4aba9ced495de06b9799c7`;
- prior gitlink:
  `44eeaecf35bdb776bbb3a7791260692f1c4e35be`;
- resulting gitlink:
  `e009f4ff1c9c4c5874e865e893cdb62e208fb2b3`;
- comparison from `b92176f911463f143f4aba9ced495de06b9799c7` through pinned engine-sim commit
  `85f7c3b959a908ed5232ede4f1a4ac7eafe6b630` shows no later change to
  `dependencies/submodules/simple-2d-constraint-solver`;
- the solver `LICENSE` at `e009f4ff1c9c4c5874e865e893cdb62e208fb2b3` is MIT,
  copyright 2022 Ange Yaghi.

NC-003B must vendor the solver only from that exact commit and must create:

```text
ThirdPartyNotices/simple-2d-constraint-solver.txt
```

containing the solver's complete MIT license text, including copyright, permission notice, and warranty disclaimer.

### 1.5 Mandatory WAV provenance and license evidence

The complete provenance record is:

```text
Repository:
ange-yaghi/engine-sim
Pinned fork:
Dziuras98/engine-sim
Pinned engine-sim commit:
85f7c3b959a908ed5232ede4f1a4ac7eafe6b630
Introduction commit:
4f7e06b211d0b51914aed0539b397ac27f70d0f3
Upstream path:
es/sound-library/new/minimal_muffling_02.wav
Git blob SHA:
6d3f8688e170cb6e5f4bfec42f580f3900514d72
Author/copyright holder:
Ange Yaghi / AngeTheGreat
License:
MIT under the engine-sim repository license
Required notice:
complete engine-sim MIT notice
```

The provenance decision is supported by the complete evidence chain, not merely by the file's location in the repository:

1. The commit introducing the WAV was authored by `ange-yaghi`.
2. The same introduction commit adds a set of impulse-response files together with authored engine configurations.
3. The repository's root `LICENSE` is the MIT license of Ange Yaghi / AngeTheGreat.
4. No separate, conflicting license was found for the sound-library directory or the WAV.
5. The user confirmed that the files were created by the engine-sim author.
6. GitHub's **Contributions Under Repository License** rule states that content added to a repository containing a license notice is licensed under the same terms unless a separate agreement supersedes it.

The resulting contract decision is that the exact WAV blob is a resolved mandatory MIT-licensed fixture. NC-003B must preserve the complete engine-sim MIT notice in `ThirdPartyNotices/engine-sim.txt`.

Resolved inputs:

- engine-sim selected source, Subaru script, and mandatory WAV: repository MIT terms;
- simple solver at the exact gitlink above: MIT;
- generated C++ impulse-response header: deterministic transformation of the pinned MIT WAV, with both source and output manifested.

Excluded inputs remain:

- `delta-studio`/`delta-basic`;
- `csv-io`;
- Piranha, UI/rendering, Discord, direct-to-video, and original application code.

NC-003A does not execute the cold-start probe and does not publish numeric cold-start values. The required measurements are an explicit deliverable of NC-003B Phase 0. No implementation agent may infer or guess them from starter specifications, source constants, recordings, or visual inspection.

NC-003A also does not publish the WAV SHA-256 or decoded audio metadata. NC-003B must calculate those values from the exact pinned blob and record them in `SOURCE_MANIFEST.json`; they must not be guessed or copied from an unverified file.

## 2. Goals and non-goals

### 2.1 First-stage goals

The first integration stage shall:

- run exactly one deterministic piston-engine fixture;
- run without the original engine-sim executable or UI;
- accept throttle and opposing-load targets;
- expose RPM and a stable telemetry snapshot;
- produce continuous PCM suitable for a real-time consumer;
- progress simulation at real-time speed when the host can sustain it;
- have explicit repeatable start, stop, restart, cancellation, failure, and destruction behavior;
- be testable without a pawn, map, game mode, or gameplay tick;
- build through UBT and a standalone headless harness;
- expose no engine-sim implementation type across the public adapter boundary;
- use a versioned, measured cold-start profile as a required build input;
- use the exact pinned Subaru EJ25 impulse-response samples through a generated compile-time C++ header.

### 2.2 Explicit non-goals

The first integration stage shall not include:

- SDL or original application UI;
- Discord Rich Presence;
- original rendering, geometry, gauges, or input handling;
- Piranha or runtime `.mr` parsing;
- runtime WAV or other fixture file I/O;
- player selection among multiple engines;
- a complete clutch, gearbox, differential, tire, or driveline model;
- replacement of `Source/Nextcar/**` movement;
- gameplay coupling between road speed and engine RPM;
- final production exhaust coloration, equalization, submix design, occlusion, attenuation, or spatial-audio tuning;
- a final CPU, latency, or buffer budget before measurement on the actual Windows runner.

The fixed `Vehicle` and `Transmission` objects required internally by `PistonEngineSimulator` remain neutral/disengaged fixture scaffolding only.

## 3. Source import and pinning strategy

### 3.1 Frozen strategy

NC-003B shall create a **minimal vendored source snapshot** from:

```text
engine-sim:
85f7c3b959a908ed5232ede4f1a4ac7eafe6b630

simple-2d-constraint-solver:
e009f4ff1c9c4c5874e865e893cdb62e208fb2b3
```

The snapshot shall:

- contain only the source and headers required by the headless simulator, synchronous synthesizer, fixed fixture, convolution/filter path, solver, and proven transitive closure;
- include the exact bytes of `es/sound-library/new/minimal_muffling_02.wav` only as a vendor-generation input and provenance artifact;
- compile through UBT as source;
- compile through the standalone Core harness;
- preserve source copyright notices;
- preserve and distribute full applicable MIT notices;
- use an allowlisted vendor generator;
- be fully described by a machine-verifiable manifest;
- contain no runtime or build-time network dependency.

Git submodules in the game repository, build-time external CMake, and checked-in prebuilt libraries remain rejected for the first slice.

### 3.2 Excluded dependency closure

`delta-studio`/`delta-basic` and `csv-io` remain excluded. The unused `delta.h` include in the selected synthesizer source shall be removed as a recorded local patch. The manifest verifier and standalone/UBT compilation must fail rather than silently expand the source closure.

### 3.3 Required provenance files and source-manifest contract

NC-003B owns:

```text
Plugins/NextcarEngineSim/Source/ThirdParty/EngineSim/
  EngineSim.Build.cs
  upstream/
  provenance/
    ENGINE_SIM_COMMIT
    SOURCE_MANIFEST.json
    PATCHES.md
    UPDATE.md
    UPSTREAM_LICENSE

ThirdPartyNotices/
  engine-sim.txt
  simple-2d-constraint-solver.txt
```

`ENGINE_SIM_COMMIT` must contain exactly:

```text
85f7c3b959a908ed5232ede4f1a4ac7eafe6b630
```

`SOURCE_MANIFEST.json` must contain at minimum:

- schema version;
- source repository URL;
- engine-sim commit;
- source branch for human context only;
- UTC generation timestamp;
- every vendored path and SHA-256;
- simple-solver repository URL and exact gitlink commit;
- every included dependency license and notice mapping;
- every local patch identifier, reason, upstream SHA-256, and patched SHA-256;
- every copied, patched, or generated fixture path and hash;
- generation-tool version.

For the impulse-response chain, the manifest must record two linked entries.

The upstream WAV entry must include:

- status `copied`;
- repository `ange-yaghi/engine-sim` and pinned fork `Dziuras98/engine-sim`;
- engine-sim commit `85f7c3b959a908ed5232ede4f1a4ac7eafe6b630`;
- introduction commit `4f7e06b211d0b51914aed0539b397ac27f70d0f3`;
- upstream path `es/sound-library/new/minimal_muffling_02.wav`;
- Git blob SHA `6d3f8688e170cb6e5f4bfec42f580f3900514d72`;
- locally calculated WAV SHA-256;
- detected WAV format, sample rate, channel count, bit depth, and sample count;
- license `MIT`;
- notice mapping to `ThirdPartyNotices/engine-sim.txt`;
- byte-for-byte copy verification.

The generated-header entry must include:

- status `generated`;
- destination path `Plugins/NextcarEngineSim/Source/NextcarEngineSimCore/Private/Generated/MinimalMuffling02ImpulseResponse.generated.h`;
- generator path `Tools/EngineSimVendor/generate_impulse_response_header.py`;
- generator SHA-256;
- exact full generation command;
- source WAV manifest reference;
- source Git blob SHA;
- source WAV SHA-256;
- generated-header SHA-256;
- detected WAV parameters;
- decoded PCM hash used by the integrity test;
- license `MIT`;
- notice mapping to `ThirdPartyNotices/engine-sim.txt`.

The WAV SHA-256 and audio metadata must be measured by NC-003B from the exact pinned file. NC-003A intentionally does not freeze unmeasured values.

### 3.4 Required update procedure

NC-003B shall add `Tools/EngineSimVendor/` and use this sequence for the initial import and every later source update:

```text
1. Create a clean temporary clone of https://github.com/Dziuras98/engine-sim.git.
2. Fetch the desired exact engine-sim commit.
3. Checkout that commit in detached-HEAD state.
4. Run: git submodule update --init --recursive
5. Verify the simple-solver gitlink is the contract pin.
6. Verify the engine-sim root MIT license and complete WAV provenance chain.
7. Copy the exact WAV bytes from the pinned upstream path and verify Git blob SHA.
8. Calculate and record the WAV SHA-256 and decoded WAV parameters.
9. Run the allowlisted vendor generator.
10. Generate MinimalMuffling02ImpulseResponse.generated.h with the frozen command.
11. Verify lossless PCM equivalence and deterministic generated-header output.
12. Generate provenance files and notices.
13. Review PATCHES.md and the complete generated diff.
14. Run the vendor verifier.
15. Run Phase 0 or Phase 1 standalone compilation and tests, as applicable.
```

A hand-edited vendored or generated file without a matching manifest entry must fail validation.

### 3.5 Accidental version-change detection

The pin is considered changed when any of the following occurs:

- `ENGINE_SIM_COMMIT` changes;
- the simple-solver commit differs from `e009f4ff1c9c4c5874e865e893cdb62e208fb2b3`;
- the WAV Git blob differs from `6d3f8688e170cb6e5f4bfec42f580f3900514d72`;
- the copied WAV SHA-256 differs from the manifest;
- decoded WAV parameters or PCM hash differ from the manifest;
- the generator path, generator hash, command, or generated-header hash differs from the manifest;
- any vendored source hash differs from the manifest;
- an included license or notice entry changes;
- a fixture/profile/generated-data hash changes;
- `GetPinnedEngineSimCommit()` differs from `ENGINE_SIM_COMMIT`.

## 4. Frozen directories and module boundaries

### 4.1 Planned tree

```text
Plugins/NextcarEngineSim/
  NextcarEngineSim.uplugin                         # manager/integration owned
  Source/
    ThirdParty/
      EngineSim/                                   # NC-003B
    NextcarEngineSimCore/                          # NC-003B
      Public/
      Private/
        Fixtures/
        Generated/
          MinimalMuffling02ImpulseResponse.generated.h
          SubaruEJ25ColdStartProfile.generated.h
    NextcarEngineSimRuntime/                       # NC-003C
      Public/
      Private/
        Tests/

Tools/
  EngineSimVendor/                                 # NC-003B
    generate_impulse_response_header.py
  EngineSimBenchmark/                              # NC-003D

Tests/
  EngineSimCore/                                   # NC-003B
    Fixtures/
      subaru_ej25_cold_start_profile.json

ThirdPartyNotices/                                 # NC-003B

.github/workflows/
  engine-sim-benchmark.yml                         # NC-003D, new path only
```

The manager shall create the shared `NextcarEngineSim.uplugin` and common module scaffold after merging NC-003A and before parallel NC-003B/C/D work begins. `Nextcar.uproject`, `Source/Nextcar/**`, the shared plugin descriptor, and shared plugin-level declarations remain manager-owned integration surfaces.

### 4.2 Module contracts

| Module/path | Responsibility | Allowed dependencies | Forbidden dependencies | Owner |
|---|---|---|---|---|
| `ThirdParty/EngineSim` | Exact minimal upstream and solver source, pinned WAV input, build rules, patches, provenance. | C++ standard library and verified vendored closure. | Gameplay, UObjects, SDL, Piranha, Discord, UI/rendering. | NC-003B |
| `NextcarEngineSimCore` | Portable fixture, generated IR header, lifecycle, synchronous simulation/synthesis, PCM conversion, telemetry, profile consumption. | ThirdParty source and C++ standard library. | Gameplay, AudioMixer, UObjects, Slate, runtime scripting or file I/O. | NC-003B |
| `NextcarEngineSimRuntime` | Unreal worker, controls, SPSC ring, `USynthComponent`, `ISoundGenerator`, fake Core. | Core public interface and Unreal runtime/audio modules. | Direct upstream headers or gameplay dependency. | NC-003C |
| `Tools/EngineSimVendor` | Deterministic import, patching, WAV parsing, header generation, manifest, hash and license verification. | Python standard library and local git for explicit update operations. | Network during ordinary build/test; silent rewrites. | NC-003B |
| `Tests/EngineSimCore` | Phase 0 calibration harness and Phase 1 standalone tests. | Portable Core/fixture and selected source closure. | Unreal, gameplay, audio device. | NC-003B |
| `Tools/EngineSimBenchmark` | Benchmark, schema, stress/soak harness, report aggregation. | Public Core interface or contract-compatible stub/fake. | Upstream types, Core internals, gameplay. | NC-003D |
| `ThirdPartyNotices` | Distribution-ready complete notices. | Verified license texts. | License assumptions. | NC-003B |

## 5. Frozen narrow C++ interface

### 5.1 Public declarations

The following declarations are normative. Mechanical export macros and include-path changes are allowed; semantics, ownership, units, and state transitions are not.

```cpp
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

namespace nextcar::enginesim
{

enum class EngineFixtureId : std::uint8_t
{
    SubaruEJ25AtgVideo2 = 1,
};

enum class LifecycleState : std::uint8_t
{
    Stopped,
    Starting,
    Running,
    Stopping,
    Failed,
};

enum class StartupFailureReason : std::uint8_t
{
    None,
    Timeout,
    NonFiniteRpm,
    StarterDisengagementNotReached,
    StallAfterStarterDisengagement,
    NoProducedPcm,
    NonFinitePcm,
    ZeroRmsAfterStart,
    StopRequested,
    SimulationError,
    InternalError,
};

enum class ErrorCode : std::uint16_t
{
    Ok = 0,
    InvalidArgument,
    InvalidState,
    UnsupportedConfiguration,
    InitializationFailed,
    SimulationFailed,
    AudioProductionFailed,
    ShutdownFailed,
    InternalError,
};

struct Status final
{
    ErrorCode code = ErrorCode::Ok;
    std::array<char, 192> message{};

    [[nodiscard]] bool ok() const noexcept { return code == ErrorCode::Ok; }
};

struct InitializationConfig final
{
    EngineFixtureId fixture = EngineFixtureId::SubaruEJ25AtgVideo2;
    std::uint32_t sample_rate_hz = 44'100;
    std::uint16_t channel_count = 1;
    std::uint32_t simulation_frequency_hz = 20'000;
    double target_synthesizer_latency_seconds = 0.1;
    std::uint32_t max_synchronous_synthesis_frames = 2'000;
    std::uint32_t max_pull_frames = 2'048;
};

struct AdvanceResult final
{
    Status status{};
    std::uint64_t simulation_steps = 0;
    double advanced_simulation_seconds = 0.0;
    std::uint32_t produced_frames = 0;
};

struct PcmPullResult final
{
    Status status{};
    std::uint32_t requested_frames = 0;
    std::uint32_t produced_frames = 0;
};

struct CoreTelemetry final
{
    LifecycleState lifecycle_state = LifecycleState::Stopped;
    ErrorCode last_error = ErrorCode::Ok;
    StartupFailureReason startup_failure_reason = StartupFailureReason::None;

    bool ignition_enabled = false;
    bool starter_enabled = false;

    double elapsed_wall_time_seconds = 0.0;
    double simulation_time_seconds = 0.0;
    double startup_elapsed_wall_seconds = 0.0;
    double startup_elapsed_simulation_seconds = 0.0;
    double starter_disengagement_rpm = 0.0;

    double current_rpm = 0.0;
    double average_rpm = 0.0;
    float throttle_target_ratio = 0.0f;
    double load_target_newton_metres = 0.0;

    std::uint32_t simulation_frequency_hz = 0;
    std::uint32_t sample_rate_hz = 0;
    std::uint16_t channel_count = 0;

    double native_synthesizer_input_latency_seconds = 0.0;
    std::uint64_t requested_native_frames = 0;
    std::uint64_t produced_native_frames = 0;
    std::uint64_t native_shortage_zero_fill_frames = 0;
    std::uint64_t requested_pcm_frames = 0;
    std::uint64_t produced_pcm_frames = 0;
    std::uint64_t pcm_non_finite_samples = 0;
    double pcm_peak_absolute = 0.0;
    double pcm_rms = 0.0;

    std::uint64_t owner_thread_violation_count = 0;
};

class IEngineSimCore
{
public:
    virtual ~IEngineSimCore() = default;

    IEngineSimCore(const IEngineSimCore&) = delete;
    IEngineSimCore& operator=(const IEngineSimCore&) = delete;
    IEngineSimCore(IEngineSimCore&&) = delete;
    IEngineSimCore& operator=(IEngineSimCore&&) = delete;

    virtual Status Start(const InitializationConfig& config) noexcept = 0;
    virtual void RequestStop() noexcept = 0;
    virtual Status SetThrottleTarget(float throttle_target_ratio) noexcept = 0;
    virtual Status SetLoadTargetNm(double load_target_newton_metres) noexcept = 0;
    virtual AdvanceResult Advance(double wall_delta_seconds) noexcept = 0;
    virtual PcmPullResult PullPcm(
        float* out_interleaved,
        std::uint32_t requested_frames) noexcept = 0;
    virtual double GetRpm() const noexcept = 0;
    virtual CoreTelemetry GetTelemetry() const noexcept = 0;
    virtual Status Stop() noexcept = 0;
};

[[nodiscard]] std::unique_ptr<IEngineSimCore> CreateEngineSimCore() noexcept;
[[nodiscard]] const char* GetPinnedEngineSimCommit() noexcept;

} // namespace nextcar::enginesim
```

There is no production `CalibrationUnavailable` behavior. The cold-start profile and generated header are mandatory versioned build inputs. A missing generated header must cause compilation failure or a deterministic validation/test failure before production execution.

### 5.2 Ownership and lifetime

- `CreateEngineSimCore()` returns the sole owning `std::unique_ptr`.
- The creating/starting owner thread is the only thread allowed to call Core methods except lock-free `RequestStop()`.
- Core owns fixture, simulator, synthesizer state, scratch buffers, converted PCM queue, generated impulse-response data, profile data, and cold-start state.
- Runtime owns Core through its dedicated worker.
- Gameplay and the Unreal audio render thread never hold upstream pointers.
- Destruction invokes `Stop()` if required; no upstream object outlives Core.

### 5.3 Exceptions and failure behavior

- The public boundary is `noexcept`.
- Invalid inputs return `InvalidArgument` and preserve prior valid targets.
- Illegal lifecycle calls return `InvalidState`.
- Non-finite RPM, simulation data, or PCM causes complete cleanup and `Failed`.
- `Start()` failure records a specific `StartupFailureReason`, disables starter then ignition, destroys native state, clears queues, and leaves no live native resource.
- Recovery is `Failed -> Stop -> Stopped -> Start`.
- No shortage zero is reported as produced PCM.

### 5.4 Units and values

- sample rate: hertz;
- simulation frequency: steps per simulated second;
- duration: seconds;
- RPM: revolutions per minute;
- throttle: finite `[0.0, 1.0]`;
- load: finite opposing dynamometer torque in N·m;
- PCM: finite normalized float32 mono;
- frame counters: audio frames, never bytes or requested-count aliases.

### 5.5 Normative `Advance()` ordering

Every successful `Advance(wall_delta_seconds)` executes on the Core owner thread:

1. read and apply the latest coherent throttle/load targets;
2. apply throttle and bounded dynamometer load;
3. call `Simulator::startFrame(wall_delta_seconds)`;
4. execute required `simulateStep()` calls;
5. call `Simulator::endFrame()` exactly once;
6. synchronously process pending synthesizer input on the same owner thread, capped by `max_synchronous_synthesis_frames`;
7. call native `readAudioOutput` into a preallocated int16 scratch buffer;
8. trust only the returned count;
9. convert exactly that returned prefix to normalized float32 and append it to the bounded Core queue;
10. exclude the shortage zero tail from production, RMS, peak, clipping, readiness, hashes, and `produced_frames`;
11. update telemetry and return.

One call is bounded by:

- finite `wall_delta_seconds` in `(0, 1/30]`;
- a conservative bounded physics-step count;
- one synchronous synthesis pass of at most 2,000 frames;
- one native read/conversion of at most 2,000 frames.

`PullPcm()` never advances simulation and never invokes synthesis.

### 5.6 PCM pull semantics

For `PullPcm(out, requested_frames)`:

- zero requested frames is a successful no-op and permits `out == nullptr`;
- otherwise `out` must hold `requested_frames * channel_count` floats;
- Core removes up to the requested number of already-produced frames;
- the returned `produced_frames` is the exact removed count;
- a caller-visible shortage tail may be zero-filled;
- shortage zeros are not added to queues, readiness, hashes, RMS, or production counters;
- the production count is the sole PCM-availability authority.

### 5.7 Automatic cold-start model

The first slice uses **automatic Core-managed cold start**. Runtime does not expose ignition or starter controls.

Lifecycle:

```text
Stopped --Start--> Starting --success--> Running --Stop--> Stopping --> Stopped
                         |
                         +--failure/timeout-----------> Failed --Stop--> Stopped

Running --fatal simulation/audio error---------------> Failed
```

`Start()` runs synchronously on the Core owner thread:

1. validate configuration, mandatory generated impulse-response header, and mandatory generated cold-start profile;
2. enter `Starting`;
3. construct the deterministic fixture, simulator, synchronous synthesizer, scratch buffers, and empty PCM queue;
4. verify the native output ring is empty;
5. apply the measured startup throttle from the generated profile;
6. enable ignition before the first simulated starter step;
7. enable starter;
8. execute bounded Core cycles in deterministic `1/120`-second slices;
9. disengage starter only when the generated profile criterion is met;
10. continue with ignition enabled for the generated stability window;
11. require RPM to remain at or above the generated post-starter minimum;
12. fail at the generated maximum startup simulation time;
13. require actual produced native frames, finite converted samples, and non-zero RMS over actual post-start PCM;
14. enter `Running` only after all checks pass.

`RequestStop()` is polled at least once per `1/120`-second cycle. Cancellation disables starter then ignition, performs complete cleanup, and ends in `Stopped`.

The following values are mandatory outputs of NC-003B Phase 0, not prerequisites for starting NC-003B:

- startup throttle;
- starter-disengagement criterion;
- post-starter minimum running RPM;
- stability-window duration;
- maximum startup simulation time.

The values may not be guessed. Absence of the profile, a mismatched hash, or values unsupported by trace evidence blocks NC-003B Phase 1 acceptance and merge.

## 6. Frozen threading and lifecycle model

### 6.1 Thread roles

| Thread | Owns/calls | Forbidden work |
|---|---|---|
| Game thread | Publishes coherent throttle/load targets and controls Runtime service lifecycle. | Upstream simulation, native PCM read, blocking audio work. |
| Runtime worker / Core owner | All Core lifecycle, fixture, physics, synchronous synthesis, native PCM read, conversion, external-ring writes. | Unreal audio callback work and hidden child threads. |
| Unreal audio render thread | Copies from the preallocated external SPSC ring and zero-fills deficits. | Core calls, allocation, mutex waits, file I/O, logging. |

Exactly one thread executes engine-sim code. Core records its owner thread and rejects or asserts any upstream operation from another thread. Core creates no child thread.

### 6.2 Mandatory synchronous synthesizer patch

NC-003B shall not call `Simulator::startAudioRenderingThread()` or `Synthesizer::startAudioRenderingThread()`.

The vendored patch shall:

1. add bounded synchronous pending-input processing;
2. invoke it only after `endFrame()` and on the owner thread;
3. use preallocated transfer buffers;
4. cap output by the supplied maximum;
5. return the actual number appended to the native ring;
6. remove or compile out the Core path's private renderer thread, waits, condition variables, and cross-thread `m_processed` protocol;
7. keep native rings, convolution/filter state, reads, writes, and destruction owner-thread-only;
8. remove the full-capacity zero-prefill loop;
9. replace process-global `rand()` with instance-owned `std::minstd_rand`;
10. seed each successful start attempt with `0x4E433033`;
11. introduce no static mutable audio state, hidden task, or child thread.

### 6.3 Minimum patch inventory

| Upstream file | Required local difference |
|---|---|
| `include/synthesizer.h` | Bounded synchronous API, no Core renderer-thread state, instance PRNG. |
| `src/synthesizer.cpp` | Empty native output initialization, synchronous processing, owner-thread ring access, no renderer thread, deterministic PRNG, no unused `delta.h`. |
| `include/simulator.h` | Synchronous processing seam and no usable native thread-start surface in the vendored Core target. |
| `src/simulator.cpp` | Synchronous wrapper and deterministic release without renderer-thread join. |
| `src/piston_engine_simulator.cpp` | Remove function-static mutable valve-lift state. |

Every modified upstream file must be marked `patched` with old/new hashes and reason. Any additional required patch must also be manifested.

### 6.4 Runtime control transfer

Runtime publishes throttle and load as one coherent latest-value snapshot. Intermediate values may be coalesced. The worker reads one snapshot before each `Advance()` and invokes Core only on the owner thread.

### 6.5 External bounded SPSC PCM ring

Frozen initial parameters:

- producer: Runtime worker;
- consumer: Unreal audio render thread;
- format: normalized float32 mono;
- capacity: 8,192 frames;
- startup prefill: 4,410 actually produced frames;
- low-water target: 4,096 frames;
- high-water target: 6,144 frames;
- maximum Core production block: 2,000 frames;
- memory allocated before audio start;
- acquire/release SPSC indices;
- unread data is never overwritten.

### 6.6 Audio readiness

Runtime exposes audible output only when:

1. Core is `Running`;
2. actual cumulative produced native frames are greater than zero;
3. all converted produced samples are finite;
4. RMS over actual post-start produced samples is greater than zero;
5. the Runtime ring holds at least 4,410 actually produced frames.

There is no native startup drain. Shortage zeros never advance readiness or prefill.

### 6.7 Underrun, overrun, stop, and destruction

- Underrun: copy available frames, zero-fill deficit, count event and frames, never block.
- Overrun: preserve unread frames, reject newest whole excess block, count event and frames, never resize.
- Stop order: stop controls, stop Unreal source, end callback consumption, signal worker, owner-thread Core stop, join worker, destroy Core, clear ring/counters.
- Restart resets fixture, PRNG seed, profile state, queues, and counters deterministically.
- Destruction is idempotent and tolerates cancellation during `Starting`.

### 6.8 Unreal procedural-audio mechanism

NC-003C shall use `USynthComponent::CreateSoundGenerator` and `ISoundGenerator`. The audio-render callback only copies from the SPSC ring and zero-fills. It does not call Core, allocate, wait on a blocking mutex, log, access files, or touch unsafe UObjects.

## 7. Audio format and buffering contract

### 7.1 Native and public formats

- native upstream output: signed int16 mono, 44,100 Hz;
- Core queue: normalized float32 mono, 44,100 Hz;
- Runtime ring: normalized float32 mono, 44,100 Hz;
- Unreal source: float32 mono with source sample rate 44,100 Hz;
- conversion: `sample / 32768.0f` or documented equivalent preserving `INT16_MIN -> -1.0`;
- conversion: Core owner thread only;
- custom resampling: out of scope.

Only the prefix identified by native returned count is converted.

### 7.2 Production bounds

Each `Advance()` performs no more than one 2,000-frame synchronous synthesis pass and one 2,000-frame native read. Zero production is a valid cycle if simulation remains valid. Partial output converts only the returned prefix.

### 7.3 Frozen Runtime buffer values

| Parameter | Value |
|---|---:|
| Runtime ring capacity | 8,192 mono frames |
| Startup prefill | 4,410 actually produced frames |
| Low-water target | 4,096 frames |
| High-water target | 6,144 frames |
| Maximum Core/native production block | 2,000 frames |
| Default maximum `PullPcm` request | 2,048 frames |

NC-003D may recommend measured tuning, but may not redefine actual-produced semantics, introduce a native thread, or count shortage zeros.

## 8. Fixed first engine configuration

### 8.1 Mechanical fixture

The mechanical fixture remains unchanged:

- name: **Subaru EJ25**;
- selector: `assets/main.mr`;
- script: `assets/engines/atg-video-2/01_subaru_ej25_eh.mr`;
- fixture ID: `SubaruEJ25AtgVideo2` / `subaru_ej25_atg_video_2_01`;
- four opposed cylinders;
- 99.5 mm bore / 79 mm stroke;
- 5.142 in connecting rod;
- starter 70 lb-ft / 500 RPM;
- redline 6,500 RPM;
- ignition limiter 6,800 RPM / 0.16 s;
- direct throttle linkage, gamma 2.0;
- simulation frequency 20,000 Hz;
- one exhaust system;
- existing mechanical intake, exhaust, cam, timing, ignition, vehicle, and transmission values.

The C++ builder is a deterministic transcription of the pinned script. Runtime Piranha parsing is forbidden. Version 1.3 restores the script's production impulse-response fixture from the exact pinned WAV while leaving the mechanical configuration unchanged.

### 8.2 Mandatory Subaru EJ25 impulse response

The mandatory production fixture is:

```text
es/sound-library/new/minimal_muffling_02.wav
```

NC-003B must copy the exact bytes from engine-sim commit `85f7c3b959a908ed5232ede4f1a4ac7eafe6b630` and verify the expected Git blob SHA:

```text
6d3f8688e170cb6e5f4bfec42f580f3900514d72
```

NC-003B must calculate, from that exact copied file:

- WAV SHA-256;
- WAV format;
- sample rate;
- channel count;
- bit depth;
- sample count.

Those measured values must be written to `SOURCE_MANIFEST.json`. They are intentionally not frozen in NC-003A without measurement.

The copied WAV is a vendor-generation and provenance input. Runtime file I/O remains forbidden. NC-003B must deterministically convert the WAV during vendor generation to:

```text
Plugins/NextcarEngineSim/Source/NextcarEngineSimCore/Private/Generated/
  MinimalMuffling02ImpulseResponse.generated.h
```

The frozen generator path is:

```text
Tools/EngineSimVendor/generate_impulse_response_header.py
```

The exact full generator command must be frozen by NC-003B in `SOURCE_MANIFEST.json` and `UPDATE.md`. It must identify the pinned WAV input, generated-header output, and any explicit deterministic options. Ordinary build, test, packaging, and runtime execution must consume only the generated header and must not open the WAV.

### 8.3 Lossless generated-header contract

`MinimalMuffling02ImpulseResponse.generated.h` must preserve the exact decoded PCM sample sequence from the pinned WAV.

The generator must not:

- normalize the material;
- trim it;
- change sample rate;
- change channel count;
- add gain;
- change sample ordering;
- silently downmix, upmix, resample, dither, compress, or otherwise alter sample values.

A necessary endian conversion or WAV-container-to-`int16_t` array conversion is permitted only when it is lossless. If the pinned WAV cannot be represented exactly as the required `int16_t` sequence, NC-003B must fail and request contract review rather than quantize or otherwise alter it. The generated sample array must be tested against PCM decoded independently from the copied WAV. The generated header must also contain the measured metadata needed to construct the engine-sim impulse response without runtime file access.

The manifest must cover:

- upstream WAV as `copied`;
- generated header as `generated`;
- generator path and SHA-256;
- exact full generation command;
- expected Git blob SHA;
- measured WAV SHA-256;
- generated-header SHA-256;
- detected WAV format, sample rate, channel count, bit depth, and sample count;
- decoded PCM hash;
- license `MIT`;
- notice mapping to `ThirdPartyNotices/engine-sim.txt`.

### 8.4 Optional identity impulse for filter unit tests

A project-owned one-sample identity impulse response may exist only as an optional unit-test fixture for isolated convolution-filter tests.

It may use one signed 16-bit sample with value `32767` to verify pass-through behavior, but it:

- is not the Subaru EJ25 production impulse response;
- is not a required part of the Subaru fixture configuration;
- must not replace `minimal_muffling_02.wav` or its generated header;
- must not satisfy production-fixture provenance, integrity, or convolution acceptance tests;
- must be separately named and manifested if checked into the repository.

### 8.5 Determinism boundary

- runtime scripting and runtime fixture file I/O are forbidden;
- process-global randomness is forbidden;
- deterministic seed is `0x4E433033`;
- native synthesis is synchronous and owner-thread-only;
- test controls and Phase 0 schedules are deterministic;
- fixture, copied WAV, generated header, profile, and local patches are manifest-verified;
- identical pinned inputs, generator, seed, configuration, and schedule must reproduce generated-header and produced-PCM hashes.

## 9. Telemetry and report schema

### 9.1 Core telemetry

The `CoreTelemetry` snapshot is mandatory. Actual produced native and PCM counts are distinct from shortage zeros. Startup telemetry records ignition, starter, elapsed simulation time, disengagement RPM, failure reason, and lifecycle.

### 9.2 Benchmark report

NC-003D shall emit UTF-8 JSON using:

```text
nextcar.engine_sim.benchmark.v1
```

Required aggregates include:

- exact engine-sim and solver commits;
- source-manifest and cold-start-profile hashes;
- fixture ID and deterministic seed;
- lifecycle/startup result;
- elapsed wall and simulation time;
- real-time factor;
- per-step and audio-production mean/p95/max;
- requested/produced native frames and shortage frames;
- requested/produced Core PCM frames;
- sample rate and channels;
- external ring fill and underrun/overrun counters;
- RPM statistics and controls;
- PCM peak, RMS, clipping, and deterministic hash;
- Core child-thread count and owner-thread violations;
- structured errors.

Nearest-rank p95 is used. Per-cycle CSV or JSON trace may be attached as a CI/PR artifact.

### 9.3 Publication rules

- Core owner thread updates mutable telemetry.
- Runtime publishes immutable snapshots.
- Audio thread updates only lock-free external underrun counters.
- Startup traces retain every cycle needed to reproduce profile decisions.
- Actual produced counts are the sole PCM-availability source.

## 10. Parallel work ownership and NC-003B phases

Parallel implementation is authorized only after:

1. NC-003A v1.3 is merged;
2. the manager creates the shared plugin scaffold.

Write scopes remain non-overlapping.

### 10.1 NC-003B Phase 0 — Fixture calibration

Phase 0 may implement only the minimum needed to produce reliable calibration evidence:

- exact vendored engine-sim source closure;
- simple solver at `e009f4ff1c9c4c5874e865e893cdb62e208fb2b3`;
- deterministic C++ Subaru EJ25 fixture;
- exact copied `minimal_muffling_02.wav` at Git blob `6d3f8688e170cb6e5f4bfec42f580f3900514d72`;
- deterministic `generate_impulse_response_header.py` and generated `MinimalMuffling02ImpulseResponse.generated.h`;
- WAV/blob/manifest/PCM integrity verification;
- synchronous synthesizer patch;
- minimal headless calibration executable;
- RPM/PCM trace collection;
- profile/header generation and validation.

Phase 0 must produce and commit:

```text
Tests/EngineSimCore/Fixtures/subaru_ej25_cold_start_profile.json

Plugins/NextcarEngineSim/Source/NextcarEngineSimCore/Private/Generated/
  MinimalMuffling02ImpulseResponse.generated.h
  SubaruEJ25ColdStartProfile.generated.h
```

The JSON cold-start profile must contain at least:

- schema version;
- engine-sim commit;
- solver commit;
- fixture identifier;
- deterministic seed;
- simulation frequency;
- calibration executable commit;
- startup throttle;
- starter-disengagement criterion;
- post-starter minimum RPM;
- stability-window duration;
- maximum startup simulation time;
- trial count;
- success count;
- startup-time distribution;
- starter-disengagement RPM distribution;
- applied margins;
- trace-data hash;
- exact command line.

The profile summary and generated header are repository inputs. Full per-cycle traces may be stored as CI/PR artifacts, but the committed profile must preserve their hash and the distributions/margins needed for review.

Phase 0 procedure remains frozen:

- verify WAV Git blob before parsing;
- calculate WAV SHA-256 and audio metadata from the exact pinned file;
- generate the compile-time impulse-response header deterministically;
- verify decoded PCM and generated array equivalence;
- ignition before the first starter step;
- synchronous model;
- `1/120`-second steps;
- candidate throttle and disengagement sweep;
- at least ten trials for the selected candidate;
- timeout negative case;
- stall-after-disengagement negative case;
- RPM and actual PCM traces;
- no guessed values.

### 10.2 NC-003B Phase 1 — Production Core

Phase 1 begins only after manager review and acceptance of the Phase 0 profile and trace evidence.

Phase 1 implements:

- `IEngineSimCore`;
- `Start()` consuming `MinimalMuffling02ImpulseResponse.generated.h` and `SubaruEJ25ColdStartProfile.generated.h`;
- complete lifecycle and cleanup;
- automatic cold start;
- PCM conversion and queue;
- telemetry;
- standalone and UBT tests;
- provenance and generated-input verification.

Missing profile, missing generated headers, mismatched profile/header hash, mismatched source/solver/WAV pin, or values unsupported by trace evidence must fail compilation or deterministic tests and block merge of NC-003B.

### 10.3 NC-003C — Runtime Audio

After the scaffold, NC-003C may work in parallel against a fake Core and owns only:

```text
Plugins/NextcarEngineSim/Source/NextcarEngineSimRuntime/**
```

It implements worker lifecycle, coherent controls, SPSC PCM ring, `USynthComponent`, `ISoundGenerator`, fake Core, and Runtime tests. It does not wait for numeric cold-start values and does not edit Core, vendored source, benchmark, project, shared plugin descriptor, workflows, or gameplay.

### 10.4 NC-003D — Measurement

After the scaffold, NC-003D may work in parallel against a contract-compatible stub/fake and owns:

```text
Tools/EngineSimBenchmark/**
.github/workflows/engine-sim-benchmark.yml
```

It implements benchmark CLI, report schema, stress/soak harness, percentile logic, fake/stub tests, and artifacts. Real-Core measurements run after NC-003B integration. It does not wait for numeric cold-start values before implementing the schema and harness.

### 10.5 Manager-owned integration surfaces

The manager owns:

- `Nextcar.uproject`;
- `Source/Nextcar/**`;
- `Plugins/NextcarEngineSim/NextcarEngineSim.uplugin`;
- shared plugin-level declarations;
- existing workflows;
- connection of real Core to Runtime;
- integration order and merge decisions;
- `docs/manager-history.md`.

## 11. Test matrix and gates

### 11.1 NC-003A validation

NC-003A is documentation-only and does not run the fixture probe or decode the WAV. It must run:

- `python scripts/validate_repository.py`;
- `git diff --check`;
- `git status --short`;
- GitHub Actions `Repository validation`.

No NC-003B/C/D test is claimed by NC-003A.

### 11.2 NC-003B Phase 0 tests

Phase 0 must execute:

| Test | Required assertion |
|---|---|
| fixture construction smoke | deterministic Subaru fixture constructs with the exact pins |
| synchronous synthesizer smoke | one bounded owner-thread pass works and creates no child thread |
| WAV Git blob verification | source path at the pinned commit has blob `6d3f8688e170cb6e5f4bfec42f580f3900514d72` |
| WAV byte-copy verification | vendored WAV bytes are identical to the pinned blob with no content change |
| WAV SHA-256 verification | locally calculated SHA-256 equals the manifest value |
| WAV parser verification | expected format is parsed without warnings and measured metadata equals the manifest |
| impulse-response non-empty | decoded impulse response has at least one sample |
| generated-header determinism | frozen generator command reproduces the exact generated-header SHA-256 |
| generated sample count | generated array has exactly the same sample count as decoded WAV PCM |
| PCM losslessness | generated-array PCM hash equals the hash of PCM decoded from the WAV |
| no IR transformation | no normalization, trimming, resampling, channel change, gain, or sample reordering occurs |
| real-IR convolution smoke | convolution with the mandatory impulse response generates finite PCM |
| no runtime WAV access | Core/fixture tests succeed with runtime WAV access blocked or the file absent from runtime location |
| deterministic output schedule | repeated fixed schedules produce the same output hash |
| throttle/disengagement sweep | candidates and outcomes are captured without guessed selection |
| selected-profile repetitions | at least ten deterministic trials for the selected candidate |
| starter disengagement traces | per-cycle RPM/starter evidence supports the criterion |
| post-starter stability traces | per-cycle RPM evidence supports the minimum and window |
| timeout negative case | deterministic timeout trace and failure result |
| stall negative case | deterministic post-disengagement stall trace and failure result |
| produced PCM count | uses actual native returned counts only |
| finite PCM | all produced converted samples are finite |
| RMS | actual produced post-start PCM has measured non-zero RMS |
| deterministic verification | repeated trace/profile hashes are verified |
| profile generation | JSON contains all mandatory fields and source hashes |
| profile header generation | generated profile header is derived from and consistent with JSON |
| profile validation | pins, command, margins, counts, distributions, trace hash, and generated hash validate |
| manifest completeness | copied WAV, generated IR header, generator, hashes, WAV parameters, licenses, and notice mapping validate |

The mandatory integrity assertions include, without substitution:

- Git blob SHA equals the expected value;
- WAV SHA-256 equals the manifest;
- parser reads the expected format without warnings;
- generated array has the same number of samples as WAV PCM;
- generated PCM hash equals the PCM hash read from the WAV;
- impulse response is not empty;
- convolution with the real impulse response generates finite PCM;
- deterministic schedule gives a repeatable output hash;
- no runtime access to the WAV occurs.

The optional one-sample identity fixture may have separate isolated convolution tests, but passing them does not satisfy any mandatory real-WAV or production-fixture assertion above.

### 11.3 NC-003B Phase 1 tests

Phase 1 must execute:

| Test | Required assertion |
|---|---|
| profile/header consistency | exact field and hash consistency; absence/mismatch fails |
| production IR header consistency | exact source blob, WAV hash, metadata, PCM hash, and generated-header consistency; absence/mismatch fails |
| automatic cold start | ignition precedes starter step and profile values drive startup |
| starter disengagement | generated criterion is applied and observed |
| post-starter stability | generated minimum/window is satisfied without starter |
| timeout cleanup | timeout disables starter/ignition and releases all native state |
| cancellation during `Starting` | clean cancellation with no deadlock and final `Stopped` |
| 100 lifecycle cycles | start/stop/restart repeats without crash, deadlock, or detectable leak |
| deterministic PCM | fixed seed/config/schedule and real IR reproduce counts and hash/statistics |
| owner-thread enforcement | zero owner-thread violations |
| no child thread | Core creates zero child threads |
| native-ring initialization | native availability is zero and there is no 44,100-frame drain |
| shortage semantics | zero tail is not produced, hashed, measured, or used for readiness |
| provenance verification | source, solver, patches, notices, copied WAV, generator, generated IR, and profile validate |
| no runtime WAV access | production Core uses only the generated header and performs no WAV file I/O |
| non-empty PCM | actual post-start produced frames satisfy readiness |
| throttle/load response | measurable response without gameplay coupling |
| sanitizer profile | ASan/UBSan/TSan where supported; unsupported configurations documented |

### 11.4 NC-003C Runtime tests

- fake-Core startup success/failure and state propagation;
- SPSC producer/consumer ordering;
- 4,410-frame actual-produced prefill;
- shortage zeros do not satisfy prefill;
- underrun and overrun handling/counters;
- callback performs no Core call, allocation, blocking wait, file I/O, or logging;
- worker cancellation during fake Core `Starting`;
- 100 Runtime lifecycle cycles;
- restart resets ring and counters;
- all relevant `Nextcar.*` Automation Tests.

### 11.5 NC-003D measurement tests

- schema validation;
- benchmark smoke against stub/fake before Core integration;
- real-Core smoke after NC-003B integration;
- actual production versus shortage accounting;
- deterministic hash/statistics;
- startup trace artifact;
- buffer stress;
- controlled underrun/overrun;
- provenance verifier invocation;
- sustained soak after full integration.

### 11.6 NC-003B merge gate

NC-003B cannot merge when any of the following is true:

- the cold-start JSON profile is absent;
- the generated profile header is absent;
- profile and header hashes disagree;
- the copied WAV is absent or differs from the pinned Git blob;
- WAV SHA-256 or detected metadata is absent or differs from the manifest;
- the generated impulse-response header is absent;
- generated sample count or PCM hash differs from the decoded WAV;
- generated-header SHA-256 differs from the manifest;
- the generator path, generator hash, or full command is absent or differs from the manifest;
- runtime code accesses the WAV;
- engine-sim or solver pin differs;
- values are present without trace hash and reviewable evidence;
- fewer than ten selected-candidate trials were executed;
- negative timeout or stall evidence is missing;
- deterministic verification fails;
- a copied, patched, or generated input is not manifested;
- required notices are incomplete.

## 12. Required integration order

1. Finalize and merge NC-003A contract version 1.3.
2. Manager creates the shared plugin scaffold.
3. Start NC-003B, NC-003C, and NC-003D in parallel within their frozen write scopes.
4. NC-003B executes Phase 0 fixture calibration, including WAV verification and deterministic header generation.
5. Manager reviews the WAV/PCM integrity evidence, cold-start profile, and trace evidence.
6. NC-003B executes Phase 1 Production Core.
7. Integrate Core.
8. Integrate benchmark and run real-Core measurement smoke.
9. Integrate Runtime with fake Core.
10. Connect real Core to Runtime.
11. Run full CI, sustained soak, and manual audio smoke.

Gate details:

- NC-003A merge requires only the frozen contract, resolved provenance decisions, documentation-only diff, and passing repository validation.
- Manager scaffold must establish shared module names without assigning overlapping files.
- Phase 0 must pass section 11.2 and produce the copied-WAV manifest entry, generated IR header, cold-start profile, and generated profile header.
- Manager acceptance must confirm exact WAV provenance, lossless PCM generation, trace-derived cold-start values, and justified margins.
- Phase 1 must pass section 11.3.
- Core integration must preserve no-child-thread, no-runtime-WAV, and actual-produced PCM semantics.
- Benchmark integration records CPU, real-time factor, and initial buffer evidence.
- Runtime fake-Core integration must pass callback-safety and SPSC tests independently.
- Real-Core connection is manager-owned.
- Full integration requires repository validation, provenance verification, standalone tests, benchmark smoke, Unreal build/tests, soak, and manual audio smoke.

A failed gate blocks the next dependent stage. Buffer enlargement cannot waive a real-time production failure. Any change to fixture source pin, WAV blob, generated impulse-response data, generated profile, boot ownership, thread model, or PCM semantics requires explicit review.

## 13. Licensing and distribution

### 13.1 License/provenance decision table

| Component/asset | Pin/source | License and notice | Status |
|---|---|---|---|
| engine-sim selected source | `85f7c3b959a908ed5232ede4f1a4ac7eafe6b630` | MIT; preserve complete engine-sim notice | **Resolved** |
| Subaru script transcription | pinned engine-sim repository MIT source | preserve upstream MIT notice and manifest source path/hash | **Resolved** |
| Subaru EJ25 impulse-response WAV | `es/sound-library/new/minimal_muffling_02.wav`, blob `6d3f8688e170cb6e5f4bfec42f580f3900514d72` | MIT under the engine-sim repository license; complete engine-sim notice required | **Resolved** |
| generated impulse-response header | deterministic lossless conversion of the pinned MIT WAV | MIT; source/output/generator hashes and notice mapping required | **Resolved** |
| optional identity filter-test fixture | project-owned scalar unit-test data, if present | project-owned/generated; separate optional test fixture only | **Optional** |
| simple 2D constraint solver | `e009f4ff1c9c4c5874e865e893cdb62e208fb2b3` | MIT, copyright 2022 Ange Yaghi; complete solver notice required | **Resolved** |
| `delta-studio` / `delta-basic` | excluded | no copied source or notice dependency | **Excluded** |
| `csv-io` | excluded | no copied source or notice dependency | **Excluded** |
| Piranha/UI/Discord/direct-to-video | excluded | no copied source or notice dependency | **Excluded** |

### 13.2 Required notices

NC-003B must create:

```text
ThirdPartyNotices/engine-sim.txt
ThirdPartyNotices/simple-2d-constraint-solver.txt
```

Each file must contain the complete applicable MIT text. Source headers must be preserved where present. Packaged distributions must include the applicable notices. The engine-sim notice covers the copied WAV and its losslessly generated C++ representation.

### 13.3 Manifest and verification rules

For every copied, patched, or generated file, `SOURCE_MANIFEST.json` records:

- origin repository and exact commit, or project-owned generator;
- upstream/source path;
- destination path;
- source Git blob SHA where applicable;
- source/upstream SHA-256 where applicable;
- destination/generated SHA-256;
- status `copied`, `patched`, or `generated`;
- patch identifier/reason where applicable;
- detected format metadata for binary assets;
- decoded PCM hash for the WAV/header chain;
- license identifier and evidence;
- notice mapping;
- generator command and generator hash for generated files.

Verification fails on missing or extra files, hash drift, unlisted patches, wrong solver pin, wrong WAV blob, changed WAV bytes, WAV/parser warnings, metadata mismatch, PCM mismatch, generated-header drift, missing notices, runtime WAV access, missing profile/header, or developer-local/network dependencies.

### 13.4 Packaging

The game packages source-compiled plugin code and approved generated data only. It does not require runtime scripting, runtime WAVs, sibling checkouts, build-time network, external CMake, or untracked prebuilts. The pinned WAV may remain in the source/provenance vendor area for reproducibility, but packaged runtime content must use only the generated header.

## 14. Frozen decisions and residual risks

### 14.1 Decision register

| Decision | Frozen selection | Consequence |
|---|---|---|
| NC-003A status | architecture/provenance decisions complete | numeric calibration and measured WAV metadata are NC-003B deliverables, not NC-003A blockers |
| Solver pin | `e009f4ff1c9c4c5874e865e893cdb62e208fb2b3` | exact source and MIT notice are mandatory |
| Production impulse response | pinned `minimal_muffling_02.wav`, blob `6d3f8688e170cb6e5f4bfec42f580f3900514d72` | exact bytes and complete provenance are mandatory |
| Runtime IR representation | losslessly generated `MinimalMuffling02ImpulseResponse.generated.h` | no runtime WAV I/O; generator and hashes are frozen in the manifest |
| Identity impulse | optional isolated convolution unit-test fixture only | cannot replace or configure the production Subaru fixture |
| Mechanical fixture | pinned Subaru EJ25 transcription | no mechanical change in version 1.3 |
| Cold-start ownership | automatic synchronous `Core::Start()` | no public ignition/starter controls |
| Cold-start numbers | generated by NC-003B Phase 0 | never guessed; mandatory build input and B merge gate |
| Profile absence | compile/test failure | no production `CalibrationUnavailable` mode |
| Native initial audio state | logically empty | no 44,100-frame prefix or drain |
| PCM availability | native returned count / actual `produced_frames` | shortage zeros excluded everywhere |
| Synthesis | bounded synchronous owner-thread pass | no native renderer thread or Core child thread |
| `Advance()` | targets -> physics -> endFrame -> synth -> actual read -> convert -> telemetry | one deterministic bounded cycle |
| `PullPcm()` | drains already-produced float PCM | never triggers physics/synthesis |
| Source import | minimal vendored source with manifest | offline deterministic UBT/standalone build |
| Randomness | per-instance `std::minstd_rand`, seed `0x4E433033` | repeatable lifecycle and PCM |
| Runtime buffer | 44.1 kHz mono float SPSC | audio callback only copies/zero-fills |
| Runtime work | may proceed against fake Core after scaffold | independent of numeric calibration |
| Measurement work | schema/harness may proceed against stub/fake after scaffold | real-Core measurements follow B integration |
| Gameplay | unchanged | later versioned powertrain integration only |

### 14.2 NC-003A blockers

There are no remaining NC-003A architecture, solver-pin, license, provenance, or mandatory-asset blockers after this version 1.3 documentation correction and passing repository validation.

Cold-start numeric values, WAV SHA-256, and decoded audio metadata are intentionally absent from this contract. They are mandatory measured outputs and merge gates of NC-003B.

### 14.3 Permitted residual risks

Residual risks after version 1.3 may concern:

- measured cold-start parameters until Phase 0 completes;
- measured WAV parameters and generated-header size until NC-003B records them;
- mean/p95/max CPU cost;
- achieved real-time factor;
- UBT/MSVC portability;
- audible quality and device source-rate conversion;
- external Runtime buffer capacity, watermarks, and scheduling;
- long-duration numerical/audio stability and soak results;
- self-hosted runner variability.

These risks may change measured/tuned values through manager review. They may not reopen automatic boot ownership, synchronous synthesis, no-child-thread ownership, native initial-buffer meaning, actual-produced PCM accounting, exact source pins, the mandatory WAV blob, lossless generated-header semantics, or the prohibition on runtime WAV I/O.

## 15. Completion and authorization rule

Version 1.3 completes NC-003A's documentation scope:

- architectural decisions are frozen;
- solver pin and MIT notice requirements are resolved;
- the Subaru EJ25 production impulse response is restored from the exact pinned WAV blob;
- the full WAV provenance and evidence chain is recorded;
- NC-003B must measure the WAV SHA-256 and audio metadata from the pinned file;
- deterministic offline conversion to `MinimalMuffling02ImpulseResponse.generated.h` is frozen;
- generated PCM must remain losslessly equivalent to decoded WAV PCM;
- runtime WAV file I/O remains forbidden;
- a one-sample identity IR is optional only for isolated filter unit tests;
- cold-start calibration remains assigned to NC-003B Phase 0;
- missing or unsupported calibration blocks NC-003B, not NC-003A;
- NC-003C and NC-003D may begin in parallel after merge and manager scaffold;
- no code, test, workflow, manifest, project, gameplay, `AGENTS.md`, or manager-history change is authorized by this PR.

PR #11 may be reviewed and merged after the documentation-only diff and `Repository validation` pass. The PR must not be self-merged by the NC-003A agent.
