# Engine-sim integration contract

Status: **Review blocked: cold-start calibration and mandatory third-party provenance are unresolved**

Contract version: **1.1**

Target project: **nEXTcAR vertical slice**

This document is the architecture and ownership contract for the first `engine-sim` integration spike. Version 1.1 corrects the native-ring startup model, freezes automatic Core-managed cold start, removes the upstream synthesizer thread from the architecture, and makes PCM production semantics explicit. NC-003B, NC-003C, and NC-003D are **not authorized to start** until the blocking calibration and provenance items identified in sections 5.7 and 13.3 are resolved with reproducible evidence and approved by the manager. No implementation agent may guess the missing values or silently substitute an asset.

## 1. Baseline and primary evidence

### 1.1 Repository baselines

| Repository | Branch | Pinned commit | Use in this contract |
|---|---|---|---|
| `Dziuras98/Nextcar` | `main` | `3ce2579f45b568e2c5fd43ee26249b561a055f1c` | Project structure, current gameplay boundary, tests, workflows, Unreal version, and integration policy. |
| `Dziuras98/engine-sim` | `master` | `85f7c3b959a908ed5232ede4f1a4ac7eafe6b630` | Core source graph, simulator and synthesizer behavior, fixed engine fixture, dependencies, and license. |

The target engine version is **Unreal Engine 5.8**. `Nextcar.uproject` at the pinned Nextcar commit declares `EngineAssociation: "5.8"`.

### 1.2 Nextcar evidence reviewed

The following files and interfaces were reviewed at the pinned Nextcar commit:

- `AGENTS.md`, in full.
- `docs/manager-history.md`, in full.
- All eleven commits reachable on `main` at the start of this task.
- All ten existing pull requests and their final states.
- `README.md`.
- `Nextcar.uproject`.
- `Source/Nextcar/Nextcar.Build.cs`.
- `Source/Nextcar/ArcadeVehicleSimulation.h`.
- `.github/workflows/repository-validation.yml`.
- `.github/workflows/unreal-ci.yml`.
- `.github/workflows/unreal-full-ci.yml`.
- `.github/workflows/delete-merged-branch.yml`.
- `Tests/ArcadeVehicleMathStandaloneTests.cpp`.
- `Source/Nextcar/Tests/ArcadeVehicleMathTests.cpp`.
- `Source/Nextcar/Tests/NextcarSmokeTests.cpp`.
- `scripts/validate_repository.py`.

Verified project facts used by this contract:

1. The existing gameplay module has no audio or third-party simulation dependency; its build dependencies are `Core`, `CoreUObject`, `Engine`, and `InputCore`.
2. The present `ArcadeVehicleSimulation` state contains arcade speed and wheel rotation, not an engine, clutch, gearbox, or driveline model.
3. The repository already has portable standalone tests, Unreal Automation Tests under `Nextcar.*`, repository validation, and an opt-in Unreal Engine 5.8 Windows self-hosted workflow.
4. The existing arcade movement is therefore an integration boundary to preserve, not the first consumer of engine-sim internals.

### 1.3 engine-sim evidence reviewed

The following files and interfaces were re-reviewed at engine-sim commit `85f7c3b959a908ed5232ede4f1a4ac7eafe6b630`:

- `LICENSE`, `.gitmodules`, root `CMakeLists.txt`, and dependency CMake files;
- `include/ring_buffer.h`;
- `include/simulator.h`, `src/simulator.cpp`;
- `include/piston_engine_simulator.h`, `src/piston_engine_simulator.cpp`;
- `include/synthesizer.h`, `src/synthesizer.cpp`;
- `include/ignition_module.h`, `src/ignition_module.cpp`;
- `include/starter_motor.h`, `src/starter_motor.cpp`;
- `include/engine.h`, `src/engine.cpp`;
- `include/throttle.h`, `include/direct_throttle_linkage.h`, `src/direct_throttle_linkage.cpp`;
- `include/dynamometer.h`, `src/dynamometer.cpp`;
- `include/transmission.h`, `include/vehicle.h`;
- `src/engine_sim_application.cpp`, especially throttle, ignition, and starter handling;
- `assets/main.mr` and `assets/engines/atg-video-2/01_subaru_ej25_eh.mr`;
- `es/sound-library/impulse_responses.mr` and the referenced `es/sound-library/new/minimal_muffling_02.wav`;
- wrapper headers `include/scs.h`, `include/delta.h`, and `include/csv_io.h`.

Verified facts used by version 1.1:

1. The root build defines a separate C++17 static target named `engine-sim`. The complete application, scripting interpreter, Discord integration, rendering, and UI are separate concerns.
2. `Simulator::readAudioOutput(int, int16_t*)` delegates to `Synthesizer::readAudioOutput`.
3. `Synthesizer::readAudioOutput` copies only the native samples currently reported by its ring, zero-fills the caller's remaining requested tail, and returns the count actually consumed. The zero-filled tail is shortage handling, not produced PCM.
4. `RingBuffer` stores only `m_writeIndex` and `m_start`; it has no occupancy count or full/empty discriminator. `Synthesizer::initialize` performs exactly `audioBufferSize` zero writes. With capacity 44,100, that full wrap returns `writeIndex` to `start`, and `RingBuffer::size()` reports zero. The native output ring therefore starts **logically empty**, not with a readable 44,100-frame zero prefix.
5. `Simulator::initializeSynthesizer` fixes native output to signed 16-bit PCM at 44,100 Hz. Input channels equal exhaust-system count, while `Synthesizer::renderAudio` sums them into one mono output sample.
6. The upstream renderer attempts at most 2,000 samples per `renderAudio()` pass.
7. `IgnitionModule::m_enabled` is initialized to `false`; ignition events are emitted only while it is enabled.
8. `StarterMotor::m_enabled` is initialized to `false`.
9. The original application maps throttle, starter, and ignition to separate controls: Q/W/E/R change throttle, S holds the starter, and A toggles ignition. Throttle alone does not start the engine.
10. The upstream synthesizer thread is not an acceptable integration boundary. Although `m_run` is atomic in the pinned header, `m_processed` is written under different mutex regimes, and the native audio ring is written after releasing `m_lock0` while `readAudioOutput` reads it under `m_lock0`; the ring indices themselves are plain `size_t`. Retaining that path would leave concurrent native-buffer access and undefined-behavior risk.
11. `Simulator::endFrame()` calls `Synthesizer::endInputBlock()`, which establishes a natural point for explicit synchronous processing on the same owner thread.
12. `src/synthesizer.cpp` includes `delta.h` but does not use delta-studio types. No selected headless Core source uses `csv_io.h`. Version 1.1 therefore excludes `delta-studio` and `csv-io` from the intended minimal copied dependency set, subject to NC-003B's manifest verifier proving the final source closure.
13. `PistonEngineSimulator` still requires the simple 2D constraint solver and internal `Engine`, `Vehicle`, and `Transmission` objects.
14. The selected fixture requests 20,000 Hz simulation and uses a 70 lb-ft, 500 RPM starter specification, but those fixture parameters do not by themselves establish safe starter-disengagement, stability-window, startup-timeout, or startup-throttle thresholds.

### 1.4 Unreal Engine 5.8 primary documentation reviewed

The Unreal-facing decision is based on Epic's Unreal Engine 5.8 documentation, not third-party examples:

- [Audio Mixer Overview](https://dev.epicgames.com/documentation/en-us/unreal-engine/audio-mixer-overview-in-unreal-engine): the Audio Mixer owns an audio render thread distinct from the audio thread; procedural sources provide 32-bit float buffers; audio rendering must be real-time safe.
- [`USynthComponent::CreateSoundGenerator`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/AudioMixer/USynthComponent/CreateSoundGenerator): creates an `ISoundGenerator` without requiring a `UObject` on the audio render thread.
- [`USynthComponent::OnGenerateAudio`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/AudioMixer/USynthComponent/OnGenerateAudio): explicitly marked as a path that is to be deprecated for new synth components in favor of `CreateSoundGenerator`.
- [`ISoundGenerator`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/ISoundGenerator): `OnGenerateAudio(float *, int32)` and generator lifecycle run for mixer buffer production.
- [`FSoundGeneratorInitParams`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/FSoundGeneratorInitParams): exposes sample rate, channel count, and callback frame sizes.
- [`USynthComponent::Initialize`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/AudioMixer/USynthComponent/Initialize): permits an explicit source sample-rate override.
- [`USoundWaveProcedural::OnGeneratePCMAudio`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/USoundWaveProcedural/OnGeneratePCMAudio) and [`USoundWaveProcedural`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/USoundWaveProcedural): confirm that procedural generation is called from the audio render path and that the legacy class uses an internal thread-safe queued-audio mechanism.

### 1.5 License and reproducibility baseline

The engine-sim repository root contains the MIT license, copyright 2022 AngeTheGreat (Ange Yaghi). The selected solver repository currently exposes an MIT license, copyright 2022 Ange Yaghi. Those facts establish the license texts found at the inspected repository paths; they do not prove the origin or redistribution rights of a separately sourced binary recording, and they do not substitute for recording the exact solver gitlink commit used by the pinned engine-sim tree.

No separate author, source-recording statement, copyright notice, or redistribution grant was found for `es/sound-library/new/minimal_muffling_02.wav`. The root MIT file alone is not treated as proof that every contributor or uploader held rights to redistribute that recording or authorize a generated C++ derivative. The WAV is therefore **not approved for copying or conversion** in this contract revision.

A reproducible cold-start probe was also attempted but could not be built in the available execution environment because the exact repository and recursive submodules could not be fetched. The actual command and result were:

```text
git ls-remote https://github.com/Dziuras98/engine-sim.git HEAD
fatal: unable to access 'https://github.com/Dziuras98/engine-sim.git/': Could not resolve host: github.com
exit_code=128
```

The GitHub connector permits source inspection but does not provide a complete checkout or an executable build surface. Consequently, version 1.1 does not invent a starter-disengagement RPM, startup throttle, stability window, or timeout. Section 5.7 defines the mandatory probe protocol and marks those values as a blocking calibration record.

## 2. Goals and non-goals

### 2.1 First-stage goals

The first integration stage shall:

- run exactly one piston-engine fixture;
- run without the original engine-sim UI or executable;
- accept a throttle target and an opposing-load target;
- expose current RPM and a stable telemetry snapshot;
- generate continuous PCM suitable for a real-time consumer;
- progress simulation at real-time speed when the host can sustain it;
- have explicit, repeatable start, stop, restart, and destruction behavior;
- be testable without a Nextcar pawn, map, game mode, or gameplay tick;
- be buildable and testable in a standalone harness as well as through Unreal Build Tool;
- expose no engine-sim implementation type across the public adapter boundary.

### 2.2 Explicit non-goals

The first integration stage shall not include:

- SDL or any original application UI;
- Discord Rich Presence;
- original application rendering, geometry, gauges, or input handling;
- Piranha or any other runtime scripting dependency in the game build;
- player selection among multiple engines;
- a complete clutch, gearbox, differential, tire, or driveline model;
- replacement of `Source/Nextcar/**` arcade movement;
- gameplay coupling between road speed and engine RPM;
- final sound mixing, production equalization, source effects, submix design, occlusion, attenuation tuning, or finished spatial audio;
- a hard CPU budget or final latency target before benchmark data exists on the actual Windows runner.

The fixed `Vehicle` and `Transmission` objects required by `PistonEngineSimulator` are internal fixture scaffolding only. They must remain in neutral/disengaged state for this spike and are not the future Nextcar powertrain contract.

## 3. Source import and pinning strategy

### 3.1 Options evaluated

| Option | Deterministic pin | UBT and packaging | Debugging | CI/environment risk | Update and license properties | Decision |
|---|---|---|---|---|---|---|
| Git submodule | Strong gitlink pin when recursively checked out. | Requires every developer, hosted job, and self-hosted runner to fetch nested repositories correctly; UBT still needs a custom source/build bridge. Packaged source provenance is split across repositories. | Good if submodules are present. | Higher risk of missing recursive checkout, credentials, detached submodule state, or unavailable network on the Windows runner. | Upstream updates are direct, but all submodule licenses still need aggregation. | Rejected for the vertical slice. |
| Vendored source snapshot | Strong when copied from a detached commit and protected by a generated manifest of paths and hashes. | Sources compile directly under UBT and are available to packaging without a network or CMake configure step. | Best source-level debugging in Visual Studio and Unreal call stacks. | Lowest dependence on developer machine tools or network after checkout. | Requires a disciplined update tool, provenance manifest, patch record, and notice preservation. | **Selected.** |
| External CMake build during Unreal build | Can pin source and CMake inputs. | Introduces a second build graph, generator/toolchain coordination, CRT/configuration matching, artifact discovery, and packaging logic outside UBT. | Split debug/build experience. | Depends on CMake, generator, compiler, environment variables, and potentially networked `FetchContent`. | Upstream layout is preserved, but reproducibility depends on the external toolchain and cache. | Rejected for the first slice. |
| Checked-in prebuilt static library | Binary can be checksum-pinned. | Easy to link for one exact Windows configuration, but every compiler, CRT, architecture, Unreal configuration, and debug-symbol variant becomes an artifact matrix. | Poor source stepping unless matching symbols and source are retained. | High ABI and toolchain drift risk; opaque to standalone sanitizers. | Redistribution and provenance must include binary build recipe and all linked licenses. | Rejected for the first slice. |

### 3.2 Frozen decision

After sections 5.7 and 13.3 are resolved and the manager authorizes implementation, NC-003B shall create a **minimal vendored source snapshot** from engine-sim commit `85f7c3b959a908ed5232ede4f1a4ac7eafe6b630` and the exact transitive source dependencies proven necessary by compilation. It shall not vendor the full original application tree by default.

The snapshot shall:

- contain only the source and headers required by the headless simulator, synthesizer, fixed fixture, and their verified transitive dependencies;
- exclude application UI/rendering, Discord, direct-to-video, Piranha runtime scripting, example engine selection, and original executable entry points;
- compile through UBT as source, not by launching CMake during an Unreal build;
- remain compilable by the standalone core test build;
- preserve original copyright headers and license texts;
- retain a machine-readable manifest that proves its origin and detects drift.

`csv-io` and `delta-basic` appear in the broad upstream CMake link list, but the reviewed headless source closure does not use `csv_io.h`, and the only selected-source `delta.h` include is unused in `src/synthesizer.cpp`. Version 1.1 therefore requires that include to be removed as a recorded patch and excludes both dependencies from the intended snapshot. The manifest verifier and standalone/UBT compile must fail rather than silently add either dependency. The simple 2D constraint solver remains required and blocked pending its exact gitlink provenance in section 13.3.

### 3.3 Required provenance files

NC-003B owns these files under the future snapshot:

```text
Plugins/NextcarEngineSim/Source/ThirdParty/EngineSim/
  EngineSim.Build.cs
  upstream/
  provenance/
    ENGINE_SIM_COMMIT
    SOURCE_MANIFEST.json
    PATCHES.md
    UPDATE.md
```

`ENGINE_SIM_COMMIT` must contain exactly:

```text
85f7c3b959a908ed5232ede4f1a4ac7eafe6b630
```

`SOURCE_MANIFEST.json` must contain, at minimum:

- schema version;
- source repository URL;
- engine-sim commit SHA;
- source branch name for human context only;
- UTC generation timestamp;
- every vendored path and its SHA-256 hash;
- every included submodule repository URL and exact gitlink commit;
- every included dependency license identifier and notice path;
- every local patch identifier and affected file hash;
- fixture source paths and hashes for every approved source or generated asset;
- generation tool version.

`PATCHES.md` must distinguish unmodified upstream files from local portability patches. A patch may not be hidden as a silent edited snapshot.

### 3.4 Exact update procedure

NC-003B shall add and own `Tools/EngineSimVendor/`. Updating the pin later must use this sequence:

```text
1. Create a clean temporary clone of https://github.com/Dziuras98/engine-sim.git.
2. Fetch the desired exact commit object.
3. Checkout that commit in detached-HEAD state.
4. Run: git submodule update --init --recursive
5. Record the engine-sim commit and every included submodule gitlink commit.
6. Verify the license of every source or asset selected for copying.
7. Run:
   python Tools/EngineSimVendor/vendor_engine_sim.py \
     --source <detached-checkout> \
     --commit <40-character-sha>
8. Review PATCHES.md and the complete generated diff.
9. Run:
   python Tools/EngineSimVendor/verify_engine_sim_vendor.py
10. Run all NC-003B standalone compilation and tests before committing.
```

The vendoring tool must use an explicit allowlist. It must not recursively copy the complete repository.

### 3.5 Accidental version-change detection

The pinned version is considered changed when any of the following occurs:

- `ENGINE_SIM_COMMIT` changes;
- a vendored source hash differs from `SOURCE_MANIFEST.json`;
- an included dependency gitlink or license entry changes;
- a fixture source hash changes;
- the compiled `GetPinnedEngineSimCommit()` string differs from `ENGINE_SIM_COMMIT`.

`verify_engine_sim_vendor.py` shall fail in all such cases unless the manifest and provenance were deliberately regenerated by the update procedure. NC-003D shall invoke this verification in CI. A hand-edited vendored file without a matching recorded patch must fail validation.

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
    NextcarEngineSimRuntime/                       # NC-003C
      Public/
      Private/
        Tests/
Tools/
  EngineSimVendor/                                 # NC-003B
  EngineSimBenchmark/                              # NC-003D
.github/workflows/
  engine-sim-benchmark.yml                         # NC-003D (new file only)
Tests/
  EngineSimCore/                                   # NC-003B
ThirdPartyNotices/                                 # NC-003B
```

The manager shall create or integrate `NextcarEngineSim.uplugin` as a shared scaffold before merging parallel implementation branches. `Nextcar.uproject`, `Source/Nextcar/**`, and shared plugin module declarations remain manager-owned integration surfaces.

### 4.2 Module contracts

| Module/path | Responsibility | Allowed dependencies | Forbidden dependencies | Public API | Unreal types allowed? | Future owner |
|---|---|---|---|---|---|---|
| `ThirdParty/EngineSim` | Pinned minimal upstream source, dependency headers/sources, provenance, UBT external/build rules. | C++ standard library; verified vendored dependencies. | Nextcar gameplay, UObjects, SDL, Piranha, Discord, original UI/rendering. | No gameplay-facing API; upstream symbols are private implementation details of Core. | No. | NC-003B |
| `NextcarEngineSimCore` | Portable adapter, fixed fixture construction, engine-sim lifecycle, native-to-public PCM conversion, core telemetry. | `ThirdParty/EngineSim`, C++ standard library. UBT module boilerplate may use Unreal build infrastructure, but portable sources may not include Unreal headers. | `Source/Nextcar/**`, `AudioMixer`, gameplay classes, UObjects, Slate, Engine UI, runtime scripting. | The interface in section 5 only. | No in public or portable implementation code. | NC-003B |
| `NextcarEngineSimRuntime` | Unreal component/facade, dedicated worker, control handoff, bounded SPSC PCM ring, `ISoundGenerator`, test double, Automation Tests. | `Core`, `CoreUObject`, `Engine`, `AudioMixer`, `NextcarEngineSimCore`. | Direct upstream engine-sim headers/types; original UI, SDL, Discord, Piranha; direct gameplay-module dependency in the first spike. | Unreal-facing component/facade and test seams; it consumes only the Core API. | Yes. | NC-003C |
| `Tools/EngineSimVendor` | Repeatable source import, manifest generation, hash verification, and provenance for manager-approved generated fixture data. | Python standard library and local git executable for update operations. | Network access during ordinary builds/tests; silent source rewriting. | Command-line tools described in section 3. | No. | NC-003B |
| `Tests/EngineSimCore` | Standalone adapter/fixture/lifecycle/PCM tests. | `NextcarEngineSimCore` portable sources and selected third-party sources. | Unreal, gameplay, audio device. | Test executable only. | No. | NC-003B |
| `Tools/EngineSimBenchmark` | Standalone benchmark, stress/soak runner, telemetry aggregation, JSON/CSV output. | Public Core API only; platform timing primitives. | Upstream engine-sim types, Unreal gameplay, direct mutation of Core internals. | CLI and report schema. | No for the required benchmark. | NC-003D |
| `ThirdPartyNotices` | Distribution-ready notices and provenance summary. | Verified license texts. | Unsupported license assumptions. | Distribution documents. | No. | NC-003B |

The Core module and its tests must not include `ArcadeCarPawn`, `ArcadeVehicleSimulation`, `NextcarGameMode`, or any future gameplay class. NC-003C shall use a fake implementation of the section 5 interface until the manager performs the real Core-to-Runtime integration.

## 5. Frozen narrow C++ interface

### 5.1 Public declarations

The following declaration is normative. Mechanical export macros and include-path adjustments are allowed; semantics, ownership, units, and state transitions are not.

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
    CalibrationUnavailable,
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
    std::array<char, 192> message{}; // UTF-8, NUL-terminated when non-empty.

    [[nodiscard]] bool ok() const noexcept { return code == ErrorCode::Ok; }
};

struct InitializationConfig final
{
    EngineFixtureId fixture = EngineFixtureId::SubaruEJ25AtgVideo2;

    // Version 1.1 supports exactly the pinned native output format.
    std::uint32_t sample_rate_hz = 44'100;
    std::uint16_t channel_count = 1;
    std::uint32_t simulation_frequency_hz = 20'000;
    double target_synthesizer_latency_seconds = 0.1;

    // One Advance performs no more than one bounded synchronous synthesis pass.
    std::uint32_t max_synchronous_synthesis_frames = 2'000;

    // Maximum caller request to PullPcm. Accepted range [1, 8,192].
    std::uint32_t max_pull_frames = 2'048;
};

struct AdvanceResult final
{
    Status status{};
    std::uint64_t simulation_steps = 0;
    double advanced_simulation_seconds = 0.0;
    std::uint32_t produced_frames = 0; // Newly converted and queued float frames.
};

struct PcmPullResult final
{
    Status status{};
    std::uint32_t requested_frames = 0;
    std::uint32_t produced_frames = 0; // Frames removed from the Core float queue.
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

    // Synchronous. Returns Ok only after automatic cold start, starter
    // disengagement, post-starter stability, and non-empty finite PCM proof.
    virtual Status Start(const InitializationConfig& config) noexcept = 0;

    // The only cross-thread Core method. It sets one atomic cancellation flag
    // and performs no upstream call, allocation, lock, logging, or cleanup.
    virtual void RequestStop() noexcept = 0;

    // Runtime controls do not expose ignition or starter in the first spike.
    virtual Status SetThrottleTarget(float throttle_target_ratio) noexcept = 0;
    virtual Status SetLoadTargetNm(double load_target_newton_metres) noexcept = 0;

    // Valid only in Running. wall_delta_seconds must be finite and in
    // (0, 1/30]. The call performs physics and one bounded synchronous audio pass.
    virtual AdvanceResult Advance(double wall_delta_seconds) noexcept = 0;

    // Pulls already-produced Core float PCM only. It never advances simulation
    // and never invokes upstream synthesis.
    virtual PcmPullResult PullPcm(
        float* out_interleaved,
        std::uint32_t requested_frames) noexcept = 0;

    virtual double GetRpm() const noexcept = 0;
    virtual CoreTelemetry GetTelemetry() const noexcept = 0;

    // Idempotent for Stopped. From Failed, Stop transitions to Stopped after
    // confirming that no native resources remain.
    virtual Status Stop() noexcept = 0;
};

[[nodiscard]] std::unique_ptr<IEngineSimCore> CreateEngineSimCore() noexcept;
[[nodiscard]] const char* GetPinnedEngineSimCommit() noexcept;

} // namespace nextcar::enginesim
```

### 5.2 Ownership and lifetime

- `CreateEngineSimCore()` returns the sole owning `std::unique_ptr`.
- The creating/starting owner thread is the only thread allowed to call Core methods, including telemetry access, except for the lock-free `RequestStop()` cancellation signal.
- Core owns the fixture, simulator, native synthesizer state, native scratch buffers, converted float queue, and cold-start state.
- Runtime owns Core through its dedicated worker. Gameplay and the Unreal audio render thread never hold upstream pointers.
- Destruction invokes `Stop()` if necessary. No upstream object may outlive Core.

### 5.3 Exceptions and failure behavior

- The public boundary is `noexcept`; exceptions must not cross it.
- Invalid input returns `InvalidArgument` and preserves the previous valid target.
- Calls in an illegal lifecycle state return `InvalidState` without mutation.
- Non-finite RPM, simulation output, or PCM causes full cleanup and `Failed`.
- `Start()` failure always returns `InitializationFailed`, records a specific `StartupFailureReason`, disables starter and ignition if constructed, destroys all fixture/simulator/synthesizer objects, clears native and converted PCM, and leaves the object in `Failed` with no live native resources.
- A failed object may be recovered only by `Stop()` (`Failed -> Stopped`) followed by a new `Start()`.
- No method logs, allocates from the Unreal audio callback, or hides failure by reporting zero-filled shortage frames as produced.

### 5.4 Units and value semantics

- sample rate: integer hertz;
- channel count: channels per frame; version 1.1 requires one;
- simulation frequency: steps per simulated second;
- wall/simulation durations: seconds;
- RPM: revolutions per minute;
- throttle target: finite ratio `[0.0, 1.0]`;
- load target: finite opposing dynamometer torque in N·m, `[0.0, 13'558.179483314]`;
- PCM: normalized float32, finite, nominal `[-1.0, 1.0]`;
- `requested_frames` and `produced_frames`: audio frames, not bytes and not scalar-count aliases;
- mono interleaving: one scalar per frame.

### 5.5 Normative `Advance()` ordering

Every successful `Advance(wall_delta_seconds)` performs exactly this owner-thread sequence:

1. atomically read/apply the latest coherent throttle/load targets supplied to Core by Runtime;
2. apply throttle through `Engine::setSpeedControl` and the load through the bounded dynamometer adapter;
3. call `Simulator::startFrame(wall_delta_seconds)`;
4. execute all required `simulateStep()` calls for that frame;
5. call `Simulator::endFrame()` exactly once;
6. synchronously process pending synthesizer input on the same owner thread, producing at most `max_synchronous_synthesis_frames` native samples;
7. call native `readAudioOutput` into a preallocated `int16_t` scratch buffer for at most that same bound;
8. trust only the integer returned by `readAudioOutput`; convert exactly that prefix to normalized float32 and append it to Core's bounded float queue;
9. ignore the native function's zero-filled shortage tail for production, RMS, peak, clipping, readiness, and `produced_frames` accounting;
10. update telemetry and return.

`Advance()` automatically produces PCM. `PullPcm()` only removes already-produced float frames. There is no alternate legal ordering for NC-003B.

The maximum work in one call is bounded by:

- `wall_delta_seconds <= 1/30`;
- simulation steps no greater than `ceil((round(wall_delta_seconds * simulation_frequency_hz) + 1) * 1.1)`, the conservative upper bound implied by the pinned low-latency adjustment; Core treats a larger upstream count as an invariant failure;
- one synchronous synthesis pass capped at 2,000 native frames;
- one native read/conversion capped at 2,000 frames.

A caller with more than `1/30` second of accumulated wall time must invoke multiple `Advance()` calls. Runtime preserves real-time factor by accumulating monotonic elapsed wall time and consuming it in bounded slices; it may not change simulated time merely to fill audio buffers. Buffer watermarks control when the worker wakes, not how much simulation time exists.

### 5.6 PCM pull and partial-production semantics

For `PullPcm(out, requested_frames)`:

- `requested_frames == 0` is a successful no-op and permits `out == nullptr`;
- otherwise `out` must hold `requested_frames * channel_count` floats;
- Core removes up to `requested_frames` already-produced frames and reports that exact number in `produced_frames`;
- if fewer frames exist, Core writes the produced prefix, zero-fills the caller-visible tail, and returns `Ok` with `produced_frames < requested_frames`;
- caller-visible shortage zeros are never appended to Core's queue, never counted as produced, and never used to satisfy startup/audio-readiness tests;
- no PCM availability is inferred from buffer contents alone; `produced_frames` is the sole availability authority.

### 5.7 Automatic cold-start state machine and blocked calibration record

The first spike uses **automatic Core-managed cold start**. Runtime does not control ignition or starter.

Normative lifecycle:

```text
Stopped --Start--> Starting --success--> Running --Stop--> Stopping --> Stopped
                         |
                         +--failure/timeout-----------> Failed --Stop--> Stopped
Running --fatal simulation/audio error---------------> Failed
```

`Start()` runs synchronously on the Core owner thread:

1. validate immutable configuration and enter `Starting`;
2. construct the deterministic fixture, simulator, synchronous synthesizer, scratch buffers, and empty Core PCM queue;
3. verify the native output ring reports zero available frames after initialization;
4. set the fixed cold-start throttle from the measured fixture profile;
5. enable ignition before the first simulated cranking step;
6. enable the starter;
7. execute the same bounded Core cycle as section 5.5 in deterministic `1/120`-second wall-delta slices;
8. when measured disengagement criteria are met, record actual RPM and disable the starter;
9. continue with ignition enabled for the measured stability window;
10. fail if RPM becomes non-finite, never reaches the disengagement criterion before the measured timeout, or falls below the measured running criterion after starter removal;
11. require at least one native read with returned count greater than zero, all converted samples finite, and RMS greater than zero over actually produced post-start samples;
12. enter `Running` and return `Ok` only after all checks pass.

`RequestStop()` may be called while `Start()` is running; the Start loop polls it at least once per 1/120-second cycle. On observation, Start records `StopRequested`, disables starter then ignition, performs full cleanup, enters `Stopped`, and returns `InitializationFailed`. Owner-thread `Stop()` from `Running` or `Failed` also disables starter and ignition before releasing objects whenever those objects still exist.

The following four fixture constants are **not calibrated in this revision and must not be guessed**:

| Cold-start constant | Required evidence | Version 1.1 value |
|---|---|---|
| startup throttle ratio | sweep proving reliable start without excessive flare | **BLOCKED — no executable probe** |
| starter-disengagement RPM/criterion | RPM trace with starter on/off transition | **BLOCKED — no executable probe** |
| post-starter stability window and minimum running criterion | RPM trace showing self-sustained operation | **BLOCKED — no executable probe** |
| maximum startup simulation time | repeated successful/failed starts plus margin rationale | **BLOCKED — no executable probe** |

Required non-published probe protocol:

- exact engine-sim commit and exact dependency gitlinks;
- deterministic fixture builder equivalent to the selected script;
- synchronous synthesizer patch described in section 6.2;
- fixed seed `0x4E433033`;
- `1/120`-second Core-cycle schedule;
- ignition enabled before the first cycle;
- candidate throttle sweep recorded per run;
- starter enabled from simulated time zero until each candidate disengagement criterion;
- RPM, ignition, starter, throttle, produced native frames, PCM RMS, and failure state sampled every Core cycle;
- at least ten repeated starts per candidate with identical hashes/statistics;
- a negative timeout case and a stall-after-disengagement case;
- JSON output containing the full RPM trace and the command/build revision.

The executed environment produced no RPM trace because the exact checkout and submodules were unavailable. Until a manager-reviewed probe supplies the four values and margins, NC-003A remains blocked and NC-003B/C/D may not begin.

## 6. Frozen threading and lifecycle model

### 6.1 Thread roles

| Thread | Owns/calls | Forbidden work |
|---|---|---|
| Game thread | publishes latest coherent throttle/load targets; creates/stops Runtime service | upstream simulation calls, native PCM reads, blocking audio work |
| Runtime worker / Core owner | all Core lifecycle, fixture objects, upstream physics, synchronous synthesis, native PCM read, int16-to-float conversion, external-ring writes | Unreal audio callback work, hidden child threads |
| Unreal audio render thread | reads preallocated external SPSC float ring into `ISoundGenerator` output and zero-fills deficits | Core/upstream calls, allocations, mutex waits, file I/O, logging |

There is exactly one thread that executes upstream engine-sim code: the Core owner thread. Core must record its owner thread identity at `Start()` and reject/assert every upstream operation attempted from another thread. NC-003B must provide a test seam or counter proving no extra thread is created.

### 6.2 Mandatory synchronous synthesizer patch

NC-003B shall not call `Simulator::startAudioRenderingThread()` or `Synthesizer::startAudioRenderingThread()`.

The vendored patch shall:

1. add a bounded `processPendingInputSynchronously(max_output_frames)` operation;
2. call it only after `endFrame()` and only on the Core owner thread;
3. move pending input into preallocated transfer buffers, render no more than the supplied bound, commit/removal deterministically, and return the actual number appended to the native audio ring;
4. remove or compile out the Core path's `m_thread`, `m_run`, condition variables, waits, and cross-mutex `m_processed` protocol;
5. ensure native input rings, filter state, native output ring, reads, writes, and destruction are all owner-thread-only;
6. remove the full-capacity zero-prefill loop so initialization leaves the native output ring explicitly/logically empty;
7. replace process-global `rand()` with an instance-owned `std::minstd_rand`, seeded `0x4E433033` on every successful `Start()` attempt;
8. introduce no process global, static mutable audio state, hidden task, or child thread.

The synchronous function must return without waiting. If no input is pending or output capacity is unavailable, it returns zero. It may process less than the bound; the returned count is authoritative.

### 6.3 Required vendored patch inventory

Every modified file must be marked `patched` in `SOURCE_MANIFEST.json`, with upstream SHA-256, patched SHA-256, reason, and patch identifier. The required minimum inventory is:

| Upstream file | Required local difference |
|---|---|
| `include/synthesizer.h` | expose bounded synchronous processing; remove/disable native thread, condition-variable, and cross-thread state from the vendored Core build; add instance PRNG state |
| `src/synthesizer.cpp` | remove 44,100 zero-prefill loop; implement synchronous bounded processing; remove native renderer-thread path; make all ring access owner-thread-only; replace `rand()`; remove unused `delta.h` include |
| `include/simulator.h` | expose the internal synchronous processing seam and remove/disable native thread start/end surface from the vendored Core target |
| `src/simulator.cpp` | route release/destruction without native thread join; provide the synchronous processing wrapper; preserve deterministic cleanup |
| `src/piston_engine_simulator.cpp` | remove the function-static mutable `lastValveLift` state and keep all per-instance/per-cycle state owned by Core |

If implementation proves another upstream file must change, it must also be marked `patched`; omission is a provenance-verifier failure. `include/ring_buffer.h` may remain byte-for-byte upstream only if the synchronous design and tests prove that no ring reaches ambiguous full capacity. Otherwise it must be patched with explicit occupancy semantics and added to this table by manager-approved contract update.

### 6.4 Control transfer

Runtime publishes throttle and load as one coherent latest-value snapshot. Intermediate values may be coalesced. The worker reads one snapshot before each `Advance()` and calls Core setters on the owner thread. There is no unbounded control queue and no gameplay dependency in Core.

### 6.5 External bounded SPSC PCM ring

The Runtime ring is separate from every upstream ring:

- producer: Runtime worker only;
- consumer: Unreal audio render thread only;
- element: one normalized float32 mono frame;
- capacity: 8,192 frames;
- startup prefill: 4,410 actually produced frames;
- low-water target: 4,096 frames;
- high-water target: 6,144 frames;
- maximum Core production block: 2,000 frames;
- memory allocated before audio starts;
- lock-free SPSC indices with documented acquire/release ordering;
- no overwrite of unread frames.

### 6.6 Audio readiness and startup publication

Runtime may expose audible output only when all of the following are true:

1. Core lifecycle is `Running`;
2. cumulative Core/native `produced_frames > 0` from actual `readAudioOutput` return values;
3. all produced float samples are finite;
4. RMS over actually produced post-start samples is greater than zero;
5. the external Runtime ring contains at least 4,410 actually produced frames.

There is no fixed native-drain step. A zero-filled shortage tail does not advance readiness or prefill.

### 6.7 Underrun and overrun

On external-ring underrun, the audio callback copies available frames, zero-fills the remainder, increments event/frame counters, and never blocks. On overrun, the worker preserves unread data, rejects the newest whole excess block, increments event/frame counters, and never resizes or overwrites. Either event fails the sustained acceptance gate.

### 6.8 Start, stop, restart, pause, and destruction

- Runtime creates its worker and Core while audio output is stopped.
- A controller stop request invokes only lock-free `Core::RequestStop()` while the worker is inside Start/Advance; owner-thread cleanup follows.
- The worker invokes synchronous `Core::Start()`; Core handles ignition/starter and returns only in `Running` or `Failed`.
- After Core succeeds, the worker fills the external ring using bounded `Advance()`/`PullPcm()` calls until section 6.6 is satisfied.
- Runtime then starts the `USynthComponent`/`ISoundGenerator` source.
- Stop order: stop accepting controls; stop/deactivate Unreal source; wait until callback no longer consumes; signal worker; worker disables starter and ignition through `Core::Stop()`; join worker; destroy Core; clear ring/counters.
- Restart constructs a fresh Core or starts a fully stopped reusable object; seed and fixture state reset identically.
- Game pause may keep the worker running only if the chosen product behavior explicitly keeps audio alive; otherwise it follows the same controlled stop. It may not freeze Core while the audio callback continues consuming indefinitely.
- Destruction is idempotent and must tolerate failure during `Starting` without deadlock.

### 6.9 Unreal procedural-audio mechanism

NC-003C shall use `USynthComponent::CreateSoundGenerator` with an `ISoundGenerator`. The generator's audio-render callback only copies from the external ring and zero-fills. It must not touch UObjects not guaranteed for that thread, call Core, allocate, lock a blocking mutex, log, or access files.

## 7. Audio format and buffering contract

### 7.1 Verified native behavior

At the pinned commit:

- output sample type: signed `int16_t`;
- output rate: 44,100 Hz;
- output channels: one mixed mono stream;
- input channels: exhaust-system count;
- native audio-ring capacity: 44,100 samples;
- upstream single render-pass target/cap: 2,000 samples;
- fixture simulation frequency: 20,000 Hz;
- `readAudioOutput(N, target)` writes N scalar values, zero-fills shortage, and returns only the count consumed from the native ring.

The initialization loop that writes exactly 44,100 zeros does **not** make 44,100 frames available. With the pinned `RingBuffer`, the full wrap makes `writeIndex == start`, and `size()` reports zero. Version 1.1 requires the misleading loop to be removed. The native output ring must test as empty immediately after initialization.

### 7.2 Core and Unreal formats

- native upstream format: int16 mono, 44,100 Hz;
- Core converted queue: normalized float32 mono, 44,100 Hz;
- Runtime SPSC ring: normalized float32 mono, 44,100 Hz;
- Unreal source format: float32 mono, source rate 44,100 Hz;
- conversion: `sample / 32768.0f` or an exactly documented equivalent preserving `INT16_MIN -> -1.0` and finite output;
- conversion location: Core owner thread only;
- custom resampling: out of scope; Unreal source conversion handles a device mixer running at another rate.

Only the first `returned_native_count` values from native scratch are converted. The native zero-filled tail is ignored. Core `produced_frames`, PCM peak/RMS, clipping, hashes, readiness, and benchmark production counts all use actual produced values only.

### 7.3 Synchronous production and partial output

Each `Advance()` performs no more than one 2,000-frame synchronous synthesis pass and one 2,000-frame native read. Results may be:

- `produced_frames > 0`: convert and queue exactly that prefix;
- `produced_frames == 0`: successful no-PCM cycle if simulation remains valid; do not fabricate production;
- partial `< requested`: convert only returned frames; record shortage separately;
- non-finite conversion or internal ring invariant failure: `AudioProductionFailed`, cleanup, `Failed`.

A run is not considered audibly started merely because calls return `Ok`. It must satisfy section 6.6.

### 7.4 External buffer parameters and tuning

| Parameter | Frozen initial value |
|---|---:|
| Runtime ring capacity | 8,192 mono frames |
| Startup prefill | 4,410 actually produced frames (100 ms) |
| Low-water target | 4,096 frames |
| High-water target | 6,144 frames |
| Maximum Core/native production block | 2,000 frames |
| Maximum default `PullPcm` request | 2,048 frames |

NC-003D may recommend changes only from callback-size, scheduling, CPU, fill-distribution, and underrun/overrun evidence. No tuning may redefine actual-produced semantics, introduce a native thread, or count shortage zeros as production.

## 8. Fixed first engine configuration

### 8.1 Selected mechanical fixture

The selected mechanical fixture remains the pinned repository default:

- name: **Subaru EJ25**;
- selector: `assets/main.mr`;
- script: `assets/engines/atg-video-2/01_subaru_ej25_eh.mr`;
- fixture ID: `SubaruEJ25AtgVideo2` / `subaru_ej25_atg_video_2_01`.

This choice is the pinned application's default, not a declaration of the final Nextcar vehicle identity.

### 8.2 Verified principal parameters

| Parameter | Pinned script value |
|---|---|
| Layout | four cylinders, two opposed banks at +90/-90 degrees |
| Bore / stroke | 99.5 mm / 79 mm |
| Connecting rod length | 5.142 in |
| Starter torque / speed | 70 lb-ft / 500 RPM |
| Engine redline | 6,500 RPM |
| Ignition limiter | 6,800 RPM, 0.16 s |
| Throttle | direct linkage, gamma 2.0 |
| Simulation frequency | 20,000 Hz |
| Exhaust systems | one |
| Audio tuning | high-frequency gain 0.01, noise 1.0, jitter 0.5 |
| Vehicle | 2,700 lb, drag 0.3, diff 3.9, tire radius 10 in |
| Transmission | 3.636, 2.375, 1.761, 1.346, 0.971, 0.756 |

### 8.3 Headless representation

NC-003B, once authorized, shall transcribe the mechanical fixture into a deterministic C++ builder that constructs all engine, cam, timing, ignition, intake/exhaust, internal vehicle/transmission, and simulator objects directly. Runtime `.mr` parsing and the Piranha interpreter are forbidden.

The builder shall initialize ignition and starter to their upstream defaults and then let `Core::Start()` perform the automatic sequence in section 5.7. It shall not bake guessed startup thresholds into the fixture.

### 8.4 Impulse-response blocker

The script references `es/sound-library/new/minimal_muffling_02.wav` through `es/sound-library/impulse_responses.mr`. No separate author/source/redistribution evidence was found for that recording. It is **not an approved mandatory input** for NC-003B and must not be copied, decoded, or converted while section 13.3 remains blocked.

A deterministic synthetic impulse response generated entirely by a documented Nextcar tool is a technically viable fallback because it avoids runtime file I/O and third-party recording provenance. It is **proposed only**; this contract does not adopt it without explicit manager approval because it changes the audio fixture.

### 8.5 Determinism boundary

- runtime scripting: forbidden;
- runtime WAV/file I/O: forbidden;
- process-global `rand()`/`srand()`: forbidden;
- seed: `0x4E433033` reset per `Start()`;
- native synthesis: synchronous owner-thread only;
- control schedule: deterministic in tests;
- fixture structural invariants and all local patches: manifest-verified;
- PCM determinism: identical seed/config/schedule must yield identical produced-frame counts and either an identical hash or explicitly approved deterministic statistics if floating-point toolchain differences prevent bit identity.

## 9. Telemetry schema

### 9.1 Core snapshot

`CoreTelemetry` in section 5 is mandatory. It reports lifecycle, startup, controls, simulation, actual native production, converted PCM, and owner-thread violations. `native_shortage_zero_fill_frames` is diagnostic only and must never be added to `produced_native_frames` or `produced_pcm_frames`.

### 9.2 Machine report

NC-003D shall emit UTF-8 JSON with schema:

```text
nextcar.engine_sim.benchmark.v1
```

Required aggregate fields:

| Field | Type | Unit/meaning |
|---|---|---|
| `schema` | string | exact schema identifier |
| `run_id` | string | unique run identifier |
| `core.repository` | string | `Dziuras98/engine-sim` |
| `core.pinned_commit` | string | exact 40-hex source pin |
| `core.source_manifest_sha256` | string | canonical manifest hash |
| `fixture.id` | string | `subaru_ej25_atg_video_2_01` |
| `lifecycle.state` | string | `Stopped/Starting/Running/Stopping/Failed` |
| `lifecycle.started` / `shutdown_clean` | boolean | lifecycle outcome |
| `lifecycle.start_status` / `shutdown_status` | string | stable status names |
| `startup.failure_reason` | string | stable `StartupFailureReason` name |
| `startup.ignition_enabled` / `starter_enabled` | boolean | final/current state |
| `startup.elapsed_simulation_seconds` | number | simulated startup duration |
| `startup.elapsed_wall_seconds` | number | monotonic startup duration |
| `startup.starter_disengagement_rpm` | number | observed RPM at starter removal |
| `elapsed_wall_seconds` | number | total monotonic run duration |
| `simulation_seconds` | number | total simulated duration |
| `real_time_factor` | number | simulation/wall duration |
| `simulation_frequency_hz` | integer | configured physics frequency |
| `simulation_iteration_us.mean/p95/max` | number | per-step wall time |
| `audio_production_us.mean/p95/max` | number | synchronous production wall time |
| `native.requested_frames` | integer | requested native frames |
| `native.produced_frames` | integer | actual native return-count sum |
| `native.shortage_zero_fill_frames` | integer | caller-tail shortage only |
| `pcm.requested_frames` / `produced_frames` | integer | Core float pull totals |
| `sample_rate_hz` / `channel_count` | integer | public PCM format |
| `ring.capacity_frames` | integer | external Runtime capacity |
| `ring.fill_frames.min/mean/max` | number | external fill observations |
| `ring.underrun_events/frames` | integer | external consumer deficits |
| `ring.overrun_events/frames` | integer | rejected external producer blocks/frames |
| `rpm.current/mean/min/max` | number | RPM observations |
| `controls.throttle_target_ratio` | number | final target |
| `controls.load_target_newton_metres` | number | final target |
| `pcm.peak_absolute` | number | actual produced normalized samples only |
| `pcm.rms` | number | actual produced normalized samples only |
| `pcm.clipped_samples` / `clipping_ratio` | integer/number | actual produced samples only |
| `determinism.seed` | integer/string | `0x4E433033` |
| `determinism.pcm_hash` | string | hash over produced frames and counts |
| `threads.created_by_core` | integer | must be zero child threads |
| `threads.owner_violation_count` | integer | must be zero |
| `errors` | array | timestamped status/failure records |

Nearest-rank p95 is used. Optional CSV contains one row per sample/cycle with wall time, simulation time, lifecycle, RPM, ignition, starter, controls, requested/produced native frames, shortage zeros, produced PCM, external fill/counters, peak/RMS, and timing.

### 9.3 Publication rules

- Core owner thread updates the mutable source telemetry.
- Runtime publishes immutable snapshots to game/HUD consumers.
- Audio render thread increments only lock-free external underrun counters and never formats JSON or logs.
- Startup traces preserve every cycle needed to reproduce disengagement and stability decisions.
- Benchmark and Runtime must use actual produced-frame counts as the sole PCM-availability source.

## 10. Non-overlapping ownership for NC-003B/C/D

### 10.1 NC-003B — Core

NC-003B exclusively owns:

- `Plugins/NextcarEngineSim/Source/ThirdParty/EngineSim/**`;
- `Plugins/NextcarEngineSim/Source/NextcarEngineSimCore/**`;
- `Tools/EngineSimVendor/**`;
- `Tests/EngineSimCore/**`;
- `ThirdPartyNotices/**`;
- pinned source/dependency provenance;
- minimal UBT and standalone Core build definitions within its owned paths;
- deterministic Subaru EJ25 C++ mechanical fixture and only manager-approved impulse-response data;
- Core interface implementation;
- synchronous owner-thread synthesizer processing and proof that Core creates no child thread;
- standalone Core tests.

NC-003B must not edit Runtime, benchmark, gameplay, project, shared plugin descriptor, workflows, or existing tests.

### 10.2 NC-003C — Runtime Audio

NC-003C exclusively owns:

- `Plugins/NextcarEngineSim/Source/NextcarEngineSimRuntime/**`;
- the dedicated Nextcar worker implementation;
- control atomics and lifecycle handoff;
- bounded SPSC float PCM ring;
- `USynthComponent` and `ISoundGenerator` implementation;
- fake Core/test double implementing the section 5 interface;
- Runtime Unreal Automation Tests.

NC-003C must compile and test against the public interface contract, not include upstream engine-sim headers. It must not edit Core, vendored source, benchmark, gameplay, project, shared plugin descriptor, workflows, or existing tests.

### 10.3 NC-003D — Measurement

NC-003D exclusively owns:

- `Tools/EngineSimBenchmark/**`;
- benchmark CLI and profiles;
- stress/soak harness;
- JSON schema and optional CSV schema under its owned tool path;
- measurement aggregation and percentile implementation;
- ring/fake-source stress tests that do not duplicate Runtime production code;
- the new `.github/workflows/engine-sim-benchmark.yml` workflow only;
- JSON/CSV/log artifact publication.

NC-003D has exclusive ownership of the new `.github/workflows/engine-sim-benchmark.yml` path. It must not edit `repository-validation.yml`, `unreal-ci.yml`, `unreal-full-ci.yml`, or `delete-merged-branch.yml`. This exact new-file assignment is non-overlapping with NC-003B and NC-003C.

### 10.4 Manager/integration reservation

The manager exclusively owns or sequences:

- `Nextcar.uproject`;
- `Source/Nextcar/**`;
- `Plugins/NextcarEngineSim/NextcarEngineSim.uplugin`;
- any shared module list or plugin-level manifest;
- all existing `.github/workflows/**` files; NC-003D owns only the new `engine-sim-benchmark.yml`;
- connection of real Core to Runtime in place of the fake;
- `docs/manager-history.md`;
- final integration order, conflict resolution, and merge decisions.

No workstream may make a temporary gameplay dependency “just to demonstrate” its code. The benchmark consumes Core directly; Runtime consumes the fake until integration; Core has no knowledge of either consumer.

## 11. Test matrix and gates

### 11.1 NC-003B Core tests required by this contract

| Test | Required assertion |
|---|---|
| native-ring initialization | native output availability is zero immediately after initialization; no 44,100-frame drain exists |
| native shortage semantics | `readAudioOutput` returned count is zero/partial as appropriate; caller zero tail is not counted or hashed as produced |
| synchronous synthesizer | pending input is processed synchronously after `endFrame`; no native renderer thread is started |
| owner-thread enforcement | every upstream operation executes on the recorded owner thread; violation count remains zero |
| no child thread | Core creates zero additional threads during start, run, stop, and failure paths |
| automatic cold start | ignition precedes first cranking step; starter engages; measured criterion disengages it |
| post-starter stability | engine remains above the measured running criterion for the measured window without starter |
| startup PCM readiness | actual produced count > 0, all samples finite, post-start RMS > 0 |
| startup timeout | blocked/starter-only case returns `InitializationFailed` with `Timeout` and full cleanup |
| stall after disengagement | returns `InitializationFailed` with `StallAfterStarterDisengagement` and full cleanup |
| clean stop while starting | cancellation disables starter/ignition, does not deadlock, ends `Stopped` |
| start failure cleanup | no live fixture/simulator/synthesizer allocations; `Failed -> Stop -> Stopped` works |
| repeated lifecycle | at least 100 consecutive start/stop/restart cycles with no deadlock, crash, or detectable leak |
| deterministic schedule | fixed control schedule and seed repeat produced counts and PCM hash/statistics |
| throttle/load response | measurable RPM response without exposing gearbox/gameplay coupling |
| non-empty PCM | only actually produced post-start frames satisfy the assertion |
| patch manifest | every locally modified upstream file is marked `patched` with old/new hashes and reason |
| license/provenance verifier | every copied file has exact repository, commit, hash, license status, and notice mapping |
| sanitizer profile | ASan/UBSan/TSan where supported by the standalone toolchain; unsupported sanitizer is documented, not reported as passed |

### 11.2 Runtime Audio tests (NC-003C, after authorization)

- fake-Core startup success/failure and state propagation;
- external SPSC producer/consumer ordering;
- 4,410-frame actual-produced prefill requirement;
- shortage zeros do not satisfy prefill;
- underrun copy/zero-fill/counters;
- overrun reject-newest/counters;
- callback performs no Core call, allocation, blocking lock, file I/O, or logging;
- worker stop during Core `Starting` and clean join;
- 100 Runtime lifecycle cycles with fake Core;
- restart resets ring and counters;
- all `Nextcar.*` Unreal Automation Tests.

### 11.3 Measurement tests (NC-003D, after authorization)

- JSON schema validation with all fields in section 9;
- benchmark smoke including automatic cold start and clean shutdown;
- actual/native production versus shortage accounting;
- deterministic hash/statistics comparison;
- startup trace artifact with disengagement/stability evidence;
- buffer stress and controlled underrun/overrun cases;
- sustained soak with zero unintended underruns/overruns;
- provenance verifier invocation and report attachment.

### 11.4 Cold-start calibration gate

Before NC-003B implementation is authorized, the temporary probe in section 5.7 must produce:

- source/dependency exact commits and build command;
- probe source or complete generation command;
- per-cycle schedule;
- at least ten successful traces per chosen candidate;
- RPM trajectory through ignition, starter engagement, starter removal, and stability window;
- produced native counts and post-start RMS;
- negative timeout and post-disengagement stall traces;
- selected four constants and explicit margins tied to measured distributions.

Current result: **BLOCKED — no executable checkout, no RPM trace, no constants selected**.

### 11.5 Short PR/CI profile

When later implementation exists, every relevant PR runs:

1. repository validator;
2. vendored source/license/provenance verifier;
3. standalone Core compile/tests;
4. deterministic fixture/cold-start/PCM tests;
5. synchronous/no-thread/owner-thread tests;
6. 100 lifecycle cycles;
7. sanitizer configuration where supported;
8. benchmark smoke and JSON schema validation;
9. `NextcarEditor Win64 Development` for Unreal-affecting changes;
10. all `Nextcar.*` Automation Tests for Runtime-affecting changes.

### 11.6 Sustained and manual gates

The Windows self-hosted soak records all section 9 fields and requires zero unintended external underruns, zero overruns, clean startup/shutdown, no crash/deadlock/detectable leak, stable finite RPM/PCM, and real-time-factor evidence. Duration remains 30 minutes for the first sustained integration gate unless the manager changes it from measured runtime cost.

Manual audio smoke occurs only after automated gates and verifies audible continuity, throttle/load response, start/stop/restart, and no startup artifact. It does not replace automated PCM or lifecycle evidence.

### 11.7 Current NC-003A validation scope

This revision is documentation-only. Repository validation and diff hygiene apply now. None of the future NC-003B/C/D tests is claimed as executed by NC-003A.

## 12. Required integration order and blocking gates

1. **Resolve NC-003A blockers without implementation work.**
   Gate: obtain the exact solver gitlink/license blob; resolve the impulse-response rights or obtain explicit approval for a deterministic synthetic replacement; run the section 5.7 probe and record its command/source, schedule, RPM traces, four selected constants, and margin rationale.

2. **Update and re-review this contract on PR #11.**
   Gate: sections 5.7 and 13.3 contain resolved evidence rather than `BLOCKED`; repository validation passes; the diff still contains only this document.

3. **Merge NC-003A.**
   Gate: manager approval and no material open decision concerning startup, PCM production, native synchronization, dependency pinning, or mandatory assets.

4. **Create the manager-owned plugin scaffold, then branch NC-003B/C/D from the same integration baseline.**
   Gate: shared `.uplugin` module names match section 4; no workstream owns overlapping files.

5. **Execute NC-003B, NC-003C, and NC-003D in parallel.**
   - B implements real Core, synchronous upstream synthesis, measured cold start, fixture, provenance, and tests.
   - C implements Runtime against fake Core.
   - D implements benchmark/reporting against a contract-compatible fake or stub, then real Core after integration.

6. **Integrate Core first.**
   Gate: source/patch/license manifest verification, standalone compile/tests, zero child threads, owner-thread proof, deterministic fixture, automatic start/stop/restart, 100 lifecycle cycles, control response, and actual non-empty PCM pass.

7. **Integrate benchmark and measurements.**
   Gate: benchmark builds against real Core, emits valid complete JSON and startup trace, and short smoke passes. Initial CPU/buffer evidence is recorded.

8. **Integrate Runtime Audio with fake Core.**
   Gate: external ring, actual-produced prefill, underrun/overrun, worker lifecycle, generator, and Automation Tests pass; audio callback safety review passes.

9. **Connect real Core to Runtime adapter.**
   Manager-owned change removes fake binding from production while retaining fake tests. No gameplay coupling is added.

10. **Run full CI and sustained test.**
    Gate: repository validation, provenance verifier, standalone tests, benchmark smoke, `NextcarEditor Win64 Development`, all `Nextcar.*` Automation Tests, complete artifacts, and 30-minute zero-underrun/zero-overrun soak pass.

11. **Run manual audio smoke.**
    Gate: audible continuity, automatic start, stop/restart, and responsive behavior are confirmed and recorded against the same build/report.

12. **Only then design and implement engine/clutch/gearbox/driveline coupling.**
    The later model may provide physical load and RPM coupling but must consume a versioned successor of the narrow interface, not upstream types.

A failed gate blocks the next stage. Buffer enlargement cannot waive a real-time production failure. A fixture, asset, source-pin, boot-profile, or thread-model change requires an explicit contract/provenance update and manager review.

## 13. Licensing and distribution

### 13.1 License/provenance decision table

| Component/asset | Repository | Pinned commit | Copyright holder | License | Evidence path | Redistribution allowed | Notice required | Status |
|---|---|---|---|---|---|---|---|---|
| engine-sim selected source and scripts | `Dziuras98/engine-sim` / upstream `ange-yaghi/engine-sim` | `85f7c3b959a908ed5232ede4f1a4ac7eafe6b630` | AngeTheGreat / Ange Yaghi, as stated in root license | MIT | root `LICENSE`; selected source/script paths in manifest | yes, under MIT conditions | full MIT copyright, permission, disclaimer | **Resolved** |
| Subaru EJ25 `.mr` source used for C++ transcription | same | same | covered by inspected repository root notice; no separate contrary notice found | MIT repository license | `assets/engines/atg-video-2/01_subaru_ej25_eh.mr`, root `LICENSE` | yes, with repository MIT notice | yes | **Resolved for transcription** |
| generated deterministic C++ mechanical fixture | Nextcar derivative of above | generated from same pin | Nextcar modifications plus upstream rights/notice | project terms plus upstream MIT notice | generator command and `SOURCE_MANIFEST.json` | yes after generator provenance passes | upstream MIT notice plus project notice | **Conditionally resolved; generator not yet implemented** |
| `minimal_muffling_02.wav` | engine-sim tree | same | **not established** | **not established for the recording** | `es/sound-library/impulse_responses.mr`; WAV path; no separate author/source/license metadata found | **no approval to copy or create derivative** | unknown | **BLOCKED — mandatory asset rejected pending rights evidence or approved fallback** |
| simple 2D constraint solver source | `ange-yaghi/simple-2d-constraint-solver` | **exact engine-sim gitlink unavailable in current environment** | Ange Yaghi in inspected repository license | MIT in inspected repository `LICENSE` | engine-sim `.gitmodules`, `include/scs.h`, solver `LICENSE` | only after exact gitlink and license blob are recorded | full solver MIT notice | **BLOCKED — license family known, exact pin/provenance unresolved** |
| `delta-studio` / `delta-basic` | `ange-yaghi/delta-studio` | not copied | n/a | n/a | only unused `src/synthesizer.cpp` include and upstream CMake link | n/a | n/a | **Excluded by required local patch/source closure** |
| `csv-io` | `ange-yaghi/csv-io` | not copied | n/a | n/a | upstream CMake link; no selected Core source reference | n/a | n/a | **Excluded from intended minimal Core closure** |
| Piranha, Discord, direct-to-video, UI/rendering, upstream GoogleTest fetch | respective sources | not copied | n/a | n/a | separate upstream targets/options | n/a | n/a | **Explicitly excluded** |
| proposed deterministic synthetic impulse response | would be generated by Nextcar | not selected | Nextcar | project license to be assigned | future generator/source | potentially yes | project notice | **Proposal only — requires manager approval; not part of v1.1 fixture** |

### 13.2 engine-sim MIT obligations

Every copied or distributed substantial portion shall preserve:

- `Copyright 2022 AngeTheGreat (Ange Yaghi)`;
- the full MIT permission notice;
- the MIT warranty disclaimer.

NC-003B shall create `ThirdPartyNotices/engine-sim.txt`, preserve notices in copied source, and include applicable notices in packaged distributions.

### 13.3 Blocking provenance findings

Two mandatory inputs are unresolved and block implementation:

1. **Impulse response:** no evidence establishes the author/source/redistribution right for `minimal_muffling_02.wav` or a generated sample-array derivative. The file must not be copied.
2. **Constraint solver pin:** the exact gitlink SHA at the engine-sim pin could not be obtained from the available connector/checkout, so the solver source cannot yet be pinned and manifested reproducibly even though an MIT license is visible in the repository inspected.

These are not permitted residual risks. The manager must resolve them by obtaining primary provenance evidence and the exact gitlink, or explicitly approve a contract change such as a deterministic synthetic impulse response. No agent may silently substitute current dependency HEADs or infer a recording license from repository placement alone.

### 13.4 Required provenance files and verifier

```text
Plugins/NextcarEngineSim/Source/ThirdParty/EngineSim/provenance/ENGINE_SIM_COMMIT
Plugins/NextcarEngineSim/Source/ThirdParty/EngineSim/provenance/SOURCE_MANIFEST.json
Plugins/NextcarEngineSim/Source/ThirdParty/EngineSim/provenance/UPSTREAM_LICENSE
ThirdPartyNotices/engine-sim.txt
ThirdPartyNotices/simple-2d-constraint-solver.txt
```

For every copied/generated file, the manifest records repository, exact commit, upstream path, destination, upstream SHA-256, destination SHA-256, `copied/patched/generated`, patch identifier/reason, license identifier/evidence path, and notice mapping. Verification fails on missing/extra files, hash drift, unlisted patches, unresolved required status, or network/developer-local dependencies.

### 13.5 Packaging rule

The game build shall package source-compiled plugin code and approved generated fixture data only. It shall not require runtime scripting, runtime WAV paths, sibling checkouts, build-time network access, external CMake, or untracked prebuilt libraries.

## 14. Frozen decisions and residual risks

### 14.1 Decision register

| Decision | Selected option | Rejected options | Evidence/consequence | Owner |
|---|---|---|---|---|
| Native initial audio state | logically empty; remove full-capacity zero-prefill | readable 44,100-zero prefix; fixed drain | ring indices wrap equal and `size()==0`; shortage zeros are not production | NC-003B |
| PCM availability authority | native return count / `produced_frames` only | buffer-content inspection; requested count; shortage tail | exact partial semantics in sections 5.6 and 7 | NC-003B/C/D |
| Cold start ownership | automatic synchronous `Core::Start()` | Runtime/manual ignition/starter controls; throttle-only start | ignition/starter default off and app controls them separately | NC-003B |
| Cold-start constants | must be measured by section 5.7 probe | guessed RPM, throttle, window, timeout | currently blocks approval | manager after probe; NC-003B consumes |
| Upstream synthesis | synchronous bounded pass on Core owner thread | native renderer thread; audio-thread physics; hidden tasks | removes concurrent native ring/state access | NC-003B |
| Native synthesizer thread | absent | retain/start/join upstream thread | no `startAudioRenderingThread()` call; zero Core child threads | NC-003B |
| `Advance()` | targets -> startFrame -> simulate -> endFrame -> synchronous synth -> actual read -> convert -> telemetry | alternative order; Pull-driven simulation | one deterministic bounded cycle | NC-003B |
| `PullPcm()` | drains already-produced float queue and zero-fills shortage | trigger synthesis/physics; count zeros as produced | audio consumer remains decoupled and bounded | NC-003B/C |
| Source import | minimal vendored snapshot with exact manifest | submodule in game repo; build-time CMake; prebuilt binary | offline UBT packaging/debugging and drift verification | NC-003B |
| Public boundary | portable exception-free `IEngineSimCore` | upstream/Unreal types in gameplay API | parallel fake/benchmark/runtime seam | NC-003B; manager versions |
| Fixture | pinned default Subaru mechanical fixture as C++ builder | runtime Piranha; arbitrary different engine | exact default identity; startup constants not guessed | NC-003B after unblock |
| Impulse response | no unproven asset may be copied | assume root MIT proves recording rights | `minimal_muffling_02.wav` remains blocked; synthetic fallback needs approval | manager |
| Randomness | instance `std::minstd_rand`, seed `0x4E433033` | process-global `rand()`/`srand()` | repeatable isolated lifecycle and PCM | NC-003B |
| Runtime PCM | 44.1 kHz mono float32 external SPSC ring | public int16, mutex/unbounded queue | UE render callback only copies/zero-fills | NC-003B/C |
| Runtime prefill | 4,410 actually produced frames | native fixed drain; shortage zeros | readiness uses actual production | NC-003C |
| Unreal source | `USynthComponent::CreateSoundGenerator` + `ISoundGenerator` | original audio device; deprecated direct path | official UE 5.8 model | NC-003C |
| Underrun/overrun | zero-fill deficit; reject newest excess; count both | block, resize, overwrite unread | real-time-safe bounded behavior | NC-003C/D |
| Gameplay | unchanged until isolated gate | B/C/D powertrain coupling | preserves current prototype | manager/later |

### 14.2 Blocking items that are not acceptable residual risks

- four cold-start constants and their RPM-trace/margin evidence;
- exact simple-solver gitlink/provenance;
- redistribution/derivative rights for the selected impulse response, or explicit manager approval of a replacement.

Until all three are closed, the contract is not ready for parallel implementation.

### 14.3 Residual risks permitted only after blockers close

After sections 5.7 and 13.3 are resolved, remaining risks may concern only:

- mean/p95/max CPU cost and real-time factor;
- audible quality and source-rate conversion;
- optimal external ring capacity, watermarks, prefill, and callback scheduling;
- UBT/MSVC portability of the minimal pinned source set;
- long-duration numerical/audio stability measured by soak;
- self-hosted runner performance variability and final CPU/latency budgets.

They may tune measured constants through manager review. They may not reopen boot semantics, native synchronization, initial-buffer meaning, produced-frame accounting, fixture source pinning, or thread ownership.

## 15. Completion and authorization rule

Version 1.1 resolves the manager's native-buffer, boot-ownership, synthesizer-threading, `Advance()`/`PullPcm()`, lifecycle, telemetry, patch-inventory, and test-matrix concerns at the architectural level. It intentionally does not fabricate unavailable cold-start measurements or third-party provenance.

NC-003B, NC-003C, and NC-003D remain blocked until a manager-reviewed update to this same contract records:

1. the exact solver gitlink and resolved notice/provenance row;
2. an approved impulse-response source with redistribution rights, or an explicitly approved deterministic synthetic replacement;
3. the cold-start probe command/source, schedule, RPM traces, selected throttle/disengagement/stability/timeout constants, and margin rationale.

Only after those facts are inserted, repository validation passes, and the corrected contract is merged may the manager-owned plugin scaffold and parallel workstreams begin. No implementation, plugin, audio runtime, benchmark, workflow, or gameplay change is authorized by this documentation revision.
