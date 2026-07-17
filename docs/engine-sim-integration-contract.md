# Engine-sim integration contract

Status: **Frozen for NC-003B, NC-003C, and NC-003D**

Contract version: **1.0**

Target: **nEXTcAR first engine-audio vertical slice**

This document is the implementation contract for the first `engine-sim` integration spike. It freezes the source pin, module boundaries, public API, fixed fixture, threading, buffering, telemetry, ownership, tests, and integration order. NC-003B, NC-003C, and NC-003D may tune only values explicitly identified as measurement-driven. They must not independently change the frozen decisions.

## 1. Baseline and primary evidence

### 1.1 Pinned baselines

| Repository | Branch | Commit |
|---|---|---|
| `Dziuras98/Nextcar` | `main` | `3ce2579f45b568e2c5fd43ee26249b561a055f1c` |
| `Dziuras98/engine-sim` | `master` | `85f7c3b959a908ed5232ede4f1a4ac7eafe6b630` |

The target engine is **Unreal Engine 5.8**. The pinned `Nextcar.uproject` declares `EngineAssociation: "5.8"`.

### 1.2 Nextcar evidence reviewed

The analysis covered the complete current `AGENTS.md`, complete `docs/manager-history.md`, all eleven commits reachable from `main`, all ten existing pull requests, and the current repository state. The following files were inspected directly:

- `README.md`;
- `Nextcar.uproject`;
- `Source/Nextcar/Nextcar.Build.cs`;
- `Source/Nextcar/ArcadeVehicleSimulation.h`;
- `.github/workflows/repository-validation.yml`;
- `.github/workflows/unreal-ci.yml`;
- `.github/workflows/unreal-full-ci.yml`;
- `.github/workflows/delete-merged-branch.yml`;
- `Tests/ArcadeVehicleMathStandaloneTests.cpp`;
- `Source/Nextcar/Tests/ArcadeVehicleMathTests.cpp`;
- `Source/Nextcar/Tests/NextcarSmokeTests.cpp`;
- `scripts/validate_repository.py`.

Verified consequences:

1. The current gameplay module depends only on `Core`, `CoreUObject`, `Engine`, and `InputCore`; it has no audio or third-party simulator dependency.
2. `ArcadeVehicleSimulation::FVehicleState` contains speed and wheel rotation only. It is not a permanent engine, clutch, gearbox, or driveline contract.
3. The repository already has portable standalone tests, `Nextcar.*` Unreal Automation Tests, repository validation, and a Windows Unreal Engine 5.8 self-hosted workflow.
4. Existing arcade movement must remain untouched during NC-003B/C/D.

### 1.3 engine-sim evidence reviewed

The analysis covered:

- default-branch commit and `LICENSE`;
- `.gitmodules`, root `CMakeLists.txt`, and dependency CMake files;
- the static `engine-sim` target and its linked targets;
- `simulator.h/.cpp`;
- `piston_engine_simulator.h/.cpp`;
- `synthesizer.h/.cpp`;
- `audio_buffer.h/.cpp`;
- engine, throttle/direct throttle linkage, dynamometer, transmission, and vehicle classes;
- the application load path in `engine_sim_application.cpp`;
- `assets/main.mr`;
- `assets/engines/atg-video-2/01_subaru_ej25_eh.mr`;
- `es/sound-library/impulse_responses.mr`;
- `es/sound-library/new/minimal_muffling_02.wav`.

Verified consequences:

1. The root build defines a C++17 static library named `engine-sim`.
2. That target links `simple-2d-constraint-solver`, `csv-io`, and `delta-basic`. Scripting, Discord, rendering, UI, and the complete application are separate targets or executable concerns.
3. `Simulator::readAudioOutput(int, int16_t*)` delegates to `Synthesizer::readAudioOutput`.
4. The synthesizer consumes available signed 16-bit samples, zero-fills the requested tail when insufficient data exists, and returns the number actually consumed from its native output ring.
5. `Simulator::initializeSynthesizer` configures a 44,100 Hz native output. Exhaust inputs are mixed into one mono output sample.
6. The native output ring has 44,100 sample slots and is initially filled with 44,100 zero samples.
7. The native renderer targets an output fill below 2,000 samples and produces at most 2,000 samples in one render pass.
8. Simulation frequency is configurable. The selected default script requests 20,000 Hz. Native output sample rate is not exposed as a configurable simulator setting at this commit.
9. The original application compiles and executes `.mr` through Piranha at runtime, but the resulting C++ object graph can be constructed without keeping the interpreter in the game.
10. `Engine::setSpeedControl` drives the fixture's direct throttle linkage.
11. `Dynamometer` can apply bounded opposing torque and is the isolated load mechanism for the spike.
12. `PistonEngineSimulator` internally requires engine, vehicle, and transmission objects; those implementation details must not escape the adapter.

### 1.4 Unreal Engine 5.8 primary documentation

The Unreal decision uses Epic documentation and source-facing API references:

- [Audio Mixer Overview](https://dev.epicgames.com/documentation/en-us/unreal-engine/audio-mixer-overview-in-unreal-engine);
- [`USynthComponent::CreateSoundGenerator`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/AudioMixer/USynthComponent/CreateSoundGenerator);
- [`USynthComponent::OnGenerateAudio`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/AudioMixer/USynthComponent/OnGenerateAudio);
- [`ISoundGenerator`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/ISoundGenerator);
- [`FSoundGeneratorInitParams`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/FSoundGeneratorInitParams);
- [`USynthComponent::Initialize`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/AudioMixer/USynthComponent/Initialize).

The frozen Unreal mechanism is `USynthComponent::CreateSoundGenerator` returning an `ISoundGenerator`. The older direct `OnGenerateAudio` override is not the selected new-synth path. The generator writes Unreal's float PCM buffer on the audio render path and must not access mutable UObjects there.

### 1.5 License baseline

The pinned engine-sim repository contains the MIT license, copyright 2022 AngeTheGreat (Ange Yaghi). The copyright notice and full permission notice must accompany all copies or substantial portions. The software is provided without warranty.

This finding applies only to engine-sim's own license. Every copied transitive dependency and asset requires separate verification before inclusion or distribution.

## 2. Goals and non-goals

### 2.1 Goals

The first stage shall:

- run exactly one piston-engine fixture;
- run without the original application UI;
- accept throttle and opposing-load targets;
- expose RPM and basic telemetry;
- generate continuous PCM in real time;
- have controlled start, stop, restart, destruction, and error behavior;
- be testable without a pawn, map, gameplay tick, or audio device;
- build as a portable core and through Unreal Build Tool;
- expose no engine-sim implementation type through its public API.

### 2.2 Non-goals

The first stage excludes:

- SDL and the original UI, renderer, gauges, and input layer;
- Discord RPC and direct-to-video support;
- Piranha or other runtime scripting in the game;
- runtime player selection among multiple engines;
- a complete clutch, gearbox, differential, tire, or driveline model;
- replacement of existing arcade movement;
- gameplay coupling between road speed and RPM;
- final mixing, equalization, effects, attenuation, occlusion, spatialization, or production audio polish;
- an invented hard CPU budget or final latency target before the first runner measurements.

The fixture's internal `Vehicle` and `Transmission` objects exist only because the pinned simulator requires them. They do not define Nextcar gameplay behavior.

## 3. Source import and pinning

### 3.1 Compared strategies

| Strategy | Determinism | UBT/CI/packaging | Debugging/update | Decision |
|---|---|---|---|---|
| Git submodule | Commit-pinned, but checkout requires recursive Git state and nested submodules. | Adds CI and developer setup failure modes; packaging still needs explicit handling. | Good source debugging; fork updates are visible. | Rejected for the first slice. |
| Vendored source snapshot | Commit and file hashes are recorded inside Nextcar. | UBT consumes repository-local sources without CMake, network, or developer checkout assumptions. | Best debugger visibility; update requires an explicit controlled import. | **Selected.** |
| External CMake build | Can be pinned, but adds a second build graph and toolchain. | Risks compiler/CRT/configuration mismatch and dependence on local CMake state. | Useful upstream, but poor as the Unreal integration contract. | Rejected. |
| Prebuilt static library | Deterministic only with a complete binary/toolchain matrix. | Requires separate binaries and symbols per target/configuration; weak portability and provenance. | Harder to debug and patch. | Rejected. |

### 3.2 Selected strategy

NC-003B shall create a **minimal vendored source snapshot**, not copy the whole repository. It shall include only the sources required by the headless simulator, the selected fixture, and the smallest proven transitive dependency set.

The snapshot shall live under:

```text
Plugins/NextcarEngineSim/Source/ThirdParty/EngineSim/
```

Required provenance files:

```text
Plugins/NextcarEngineSim/Source/ThirdParty/EngineSim/provenance/ENGINE_SIM_COMMIT
Plugins/NextcarEngineSim/Source/ThirdParty/EngineSim/provenance/SOURCE_MANIFEST.json
Plugins/NextcarEngineSim/Source/ThirdParty/EngineSim/provenance/UPSTREAM_LICENSE
ThirdPartyNotices/engine-sim.txt
```

`ENGINE_SIM_COMMIT` shall contain exactly:

```text
85f7c3b959a908ed5232ede4f1a4ac7eafe6b630
```

`SOURCE_MANIFEST.json` shall contain, for every vendored or generated file: logical upstream repository, exact commit, upstream path, destination path, SHA-256, license identifier or review status, and whether the file is copied, patched, or generated.

No build may fetch engine-sim or a dependency from the network. No build may depend on a sibling checkout or developer environment variable.

### 3.3 Update procedure and drift detection

Updating the pin is a manager-reviewed operation:

1. Create a dedicated update branch.
2. Record the proposed engine-sim commit and each required dependency commit.
3. Re-run the dependency-reachability analysis from the headless target.
4. Re-import only the approved manifest paths.
5. Regenerate deterministic generated artifacts, including the fixture and impulse-response array.
6. Recompute SHA-256 entries.
7. Update all applicable license and notice files.
8. Run core tests, benchmark smoke, Unreal build/tests, sustained test, and manual audio smoke.
9. Review the upstream diff between old and new pins.
10. Merge only after the contract or provenance change is explicitly approved.

NC-003B shall provide `Tools/EngineSimVendor/update_engine_sim.py` and `Tools/EngineSimVendor/verify_engine_sim.py`. The verifier must fail when the recorded commit differs, a manifest file is missing or extra, or any SHA-256 differs. Repository validation or a dedicated third-party verification step must run it. An unrecorded source change is a build/CI failure, not an acceptable local patch.

## 4. Frozen directories and modules

```text
Plugins/NextcarEngineSim/
  NextcarEngineSim.uplugin                 # manager-owned shared manifest
  Source/
    ThirdParty/EngineSim/                  # NC-003B
    NextcarEngineSimCore/                  # NC-003B
    NextcarEngineSimRuntime/               # NC-003C
Tools/
  EngineSimVendor/                         # NC-003B
  EngineSimBenchmark/                      # NC-003D
Tests/
  EngineSimCore/                           # NC-003B
ThirdPartyNotices/                         # NC-003B
```

| Module/component | Responsibility | Allowed dependencies | Forbidden dependencies | Public API | Unreal types | Owner |
|---|---|---|---|---|---|---|
| `ThirdParty/EngineSim` | Pinned minimal upstream sources, local portability patches, fixture data, provenance. | C++ standard library and explicitly manifested minimal dependencies. | SDL, Piranha runtime, Discord, rendering/UI, gameplay. | None outside Core. | No. | NC-003B |
| `NextcarEngineSimCore` | Portable adapter, fixture construction, lifecycle, control application, simulation advancement, native-to-float conversion, telemetry. | Third-party snapshot, C++ standard library. | Gameplay classes, UObjects, Audio Mixer, engine-sim types in public headers. | Section 5. | No. | NC-003B |
| `NextcarEngineSimRuntime` | Unreal lifecycle, worker, control handoff, SPSC PCM ring, synth component/generator, fake core, Runtime Automation Tests. | Core public headers, Unreal `Core`, `Engine`, `AudioMixer` as proven by implementation. | Upstream private headers, scripting/UI/Discord, gameplay module internals. | Unreal-facing component/service plus Core interface consumption. | Yes. | NC-003C |
| `EngineSimBenchmark` | Standalone benchmark, stress/soak harness, JSON/CSV output, schema validation. | Core public headers and standard library. | UObjects, gameplay, audio device. | CLI/report schema. | No. | NC-003D |

Core and its testable adapter shall compile without `Source/Nextcar/**` and without Unreal headers.

## 5. Frozen public C++ interface

The public Core boundary shall be equivalent to the following portable C++20 API. Names may receive export macros or namespace formatting required by UBT, but semantics, types, units, and ownership shall not change.

```cpp
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>

namespace nextcar::engine_sim {

enum class StatusCode : std::uint8_t {
    Ok,
    InvalidArgument,
    InvalidState,
    InitializationFailed,
    SimulationFailed,
    AudioFailed,
    ShutdownFailed
};

struct Status final {
    StatusCode code{StatusCode::Ok};
    std::string_view message{}; // valid until the next non-const call on this object
    [[nodiscard]] bool ok() const noexcept { return code == StatusCode::Ok; }
};

struct InitializationConfig final {
    std::uint32_t output_sample_rate_hz{44100};
    std::uint32_t output_channel_count{1};
    std::uint32_t requested_block_frames{2048};
    std::uint32_t target_latency_frames{4410};
    std::uint32_t simulation_frequency_hz{20000};
    std::uint32_t deterministic_seed{0x4E433033u};
};

struct ControlTargets final {
    float throttle_normalized{0.0f};       // [0, 1]
    double opposing_load_newton_metres{0}; // [0, 13558.179483314]
};

struct PcmPullResult final {
    std::uint32_t requested_frames{0};
    std::uint32_t produced_frames{0};
    Status status{};
};

struct CoreTelemetry final {
    double elapsed_wall_seconds{0};
    double simulation_seconds{0};
    double real_time_factor{0};
    std::uint32_t simulation_frequency_hz{0};
    std::uint64_t simulation_steps{0};
    std::uint64_t requested_audio_frames{0};
    std::uint64_t produced_audio_frames{0};
    std::uint32_t sample_rate_hz{0};
    std::uint32_t channel_count{0};
    double current_rpm{0};
    double throttle_target_normalized{0};
    double load_target_newton_metres{0};
    StatusCode state{StatusCode::InvalidState};
};

class IEngineSimCore {
public:
    virtual ~IEngineSimCore() = default;

    virtual Status Start(const InitializationConfig& config) noexcept = 0;
    virtual Status SetControlTargets(const ControlTargets& targets) noexcept = 0;
    virtual Status Advance(double wall_delta_seconds) noexcept = 0;
    virtual PcmPullResult PullPcm(std::span<float> interleaved_output) noexcept = 0;
    [[nodiscard]] virtual double GetRpm() const noexcept = 0;
    [[nodiscard]] virtual CoreTelemetry GetTelemetry() const noexcept = 0;
    virtual Status Stop() noexcept = 0;
};

[[nodiscard]] std::unique_ptr<IEngineSimCore> CreateEngineSimCore() noexcept;

} // namespace nextcar::engine_sim
```

### 5.1 Ownership and lifetime

- `CreateEngineSimCore` transfers exclusive ownership through `std::unique_ptr`.
- One owner thread may call all non-const methods. Concurrent non-const calls are invalid.
- `Start` creates all simulator resources and starts any private native synthesizer thread.
- Configuration is immutable between successful `Start` and `Stop`.
- `Stop` is idempotent. It stops and joins internal threads before destroying simulator, engine, vehicle, transmission, fixture, filters, and buffers.
- Destruction after `Stop`, failed `Start`, or without `Start` is safe. The destructor performs a final non-throwing stop if needed.
- Restart means `Stop`, then `Start` on the same object. A failed restart leaves the object stopped.

### 5.2 Errors and exceptions

- No public function throws.
- All exceptions from allocation, standard library operations, or adapted upstream code are caught at the Core boundary and converted to `Status`.
- Invalid inputs are rejected without mutating the active state.
- A fatal simulation/audio error transitions the object to a failed/stopped state. Subsequent `Advance`/`PullPcm` return `InvalidState` until `Stop` and a new `Start`.
- Assertions are not the public error mechanism.
- Error-message storage is owned by the Core object and performs no allocation on the Unreal audio render thread because that thread never calls Core.

### 5.3 Units, ranges, and PCM semantics

- Time: seconds, finite, monotonic where applicable.
- RPM: revolutions per minute, non-negative finite `double`.
- Throttle: normalized `[0, 1]`; out-of-range or non-finite values return `InvalidArgument`.
- Load: absolute opposing dynamometer torque in N·m, `[0, 13558.179483314]`. The upper bound equals the pinned dynamometer default of 10,000 lb-ft converted to N·m. It is not a clutch or gearbox model.
- `Advance` accepts finite `wall_delta_seconds` in `(0, 0.25]`; longer host gaps are split by the caller into bounded calls.
- Public PCM is normalized `float`, nominally `[-1, 1]`, 44,100 Hz, mono.
- `interleaved_output.size()` counts scalar samples. Frames equal `size / channel_count`; the size must be divisible by the configured channel count.
- Mono is trivially interleaved. Future multi-channel support would use frame-major channel interleaving, but channel count is fixed to one in this contract.
- `PullPcm` never waits for future samples. It writes only `produced_frames * channel_count` samples and leaves the remainder unchanged. The caller applies its own underrun policy.
- `PullPcm` with an empty span is a successful no-op.
- Native signed `int16_t` is converted on the Core owner/Runtime worker thread using `float(sample) / 32768.0f`; `INT16_MIN` maps to `-1.0f` and `INT16_MAX` remains below `1.0f`.
- The native zero-filled tail is not counted as produced. The adapter uses the native return value to report partial production.

## 6. Frozen threading and lifecycle model

### 6.1 Thread ownership

1. The game thread owns the Runtime service/component and publishes latest control targets.
2. One dedicated Nextcar worker thread exclusively owns `IEngineSimCore` and all engine-sim physics state.
3. The pinned engine-sim synthesizer may retain its private rendering thread inside Core for this spike. Core starts, signals, joins, and destroys it; Runtime never addresses it directly.
4. One preallocated bounded SPSC float ring has the worker as sole producer and the Unreal `ISoundGenerator` audio render callback as sole consumer.
5. The Unreal audio render thread reads ready float frames only. It never invokes engine-sim physics, Core lifecycle, file I/O, logging, allocation, waits, locks, or gameplay/UObject access.

### 6.2 Control handoff

NC-003C shall use a latest-value mailbox, not an unbounded event queue. Throttle and load are published as one coherent versioned pair. The worker reads a stable pair and applies it before the next `Advance`. Intermediate game-frame values may be coalesced; the newest complete pair wins.

No game-thread call may directly enter Core while the worker owns it.

### 6.3 Runtime start

The manager/Runtime integration shall execute:

1. Construct Runtime service and preallocate worker state and the 8,192-frame SPSC ring.
2. Create the Core or fake Core.
3. Start the worker.
4. On the worker, call `Start` with the frozen configuration.
5. Drain exactly the native initial 44,100-sample zero prefix before declaring audible readiness; those zeros are startup priming, not generated engine signal.
6. Produce until the Runtime ring reaches the 4,410-frame prefill target.
7. Publish `Running` only after Core start, native drain, and Runtime prefill succeed.
8. Start/activate the Unreal synth component and generator.

A timeout or Core error aborts startup, joins the worker, clears the ring, reports failure, and leaves no running audio source.

### 6.4 Runtime stop, restart, and destruction

Stop order is mandatory:

1. Game thread transitions Runtime to `Stopping` and prevents new start/restart requests.
2. Deactivate the synth source and signal the generator to return silence.
3. Signal the worker to stop.
4. Worker completes its current bounded operation, calls Core `Stop`, then exits.
5. Game thread joins the worker outside the audio render callback.
6. Destroy generator/shared ring references only after audio consumption has ceased.
7. Clear ring/control/telemetry state and transition to `Stopped`.

Restart is a full stop followed by a fresh start and fresh deterministic fixture state. Destruction always performs this stop sequence. No worker or native synthesizer thread may outlive its owner.

When gameplay is paused, the default first-slice policy is to publish throttle `0` and load `0` while allowing audio production to continue. World teardown invokes Stop. A later pause policy requires a separate product decision.

### 6.5 Underrun and overrun behavior

- **Underrun:** the generator consumes all available frames, zero-fills the missing callback tail, increments underrun event and missing-frame counters, and returns the full requested callback sample count. It never blocks or calls the producer.
- **Overrun:** the producer performs an all-or-nothing write per production block. When free capacity is smaller than the block, it preserves unread old frames, rejects the newest whole block, and increments overrun event and rejected-frame counters. It does not overwrite, resize, or block.
- Any underrun or overrun fails the sustained acceptance test. Counters still exist to diagnose and tune the system.

Implementation ownership: NC-003B owns Core and its private synth lifecycle; NC-003C owns Runtime worker/mailbox/ring/generator/lifecycle; NC-003D owns measurement and stress verification.

## 7. Audio format and buffering

### 7.1 Verified native format

At the pinned commit:

- native sample type: signed `int16_t`;
- native output sample rate: 44,100 Hz;
- native output channels: one mixed mono stream;
- native output ring allocation: 44,100 samples;
- native startup content: 44,100 zero samples;
- native render pass/threshold: up to 2,000 samples;
- target synthesizer input latency default: 0.1 seconds;
- selected fixture simulation frequency: 20,000 Hz;
- `readAudioOutput(N, target)` writes N values, zero-fills shortage, and returns the consumed available count.

`audio_buffer.*` is an application-side helper and does not change the `Simulator`/`Synthesizer` output contract.

### 7.2 Adapter and Unreal formats

- Core internal adapter format after conversion: normalized float32 mono at 44,100 Hz.
- Runtime ring format: the same float32 mono frames.
- Unreal generator format: float32 mono source at 44,100 Hz.
- The synth component shall request/initialize 44,100 Hz. If the device mixer runs at another rate, Unreal's source conversion remains outside Core.
- No custom resampler is implemented in the first spike.
- Conversion occurs on the worker before writing the Runtime ring, never in the audio render callback.

### 7.3 Initial Runtime buffer parameters

| Parameter | Frozen initial value |
|---|---:|
| Ring capacity | 8,192 mono frames |
| Startup prefill | 4,410 frames (100 ms) |
| Producer block | 2,048 frames maximum |
| Low-water target | 4,096 frames |
| High-water target | 6,144 frames |

The worker advances and pulls in bounded blocks while fill is below the high-water target. It stops producing when fill reaches or exceeds high water and resumes when consumption drops below low water.

NC-003D may recommend changing capacity, prefill, low/high water, or block size only from recorded callback-size, scheduling, CPU, fill-distribution, and underrun/overrun data. Any accepted change must update the contract/provenance or a manager-owned integration record; an agent may not silently tune constants.

## 8. Fixed first engine fixture

### 8.1 Selected configuration

The fixture is the pinned repository's own default:

- name: **Subaru EJ25**;
- script: `assets/engines/atg-video-2/01_subaru_ej25_eh.mr`;
- selected by: `assets/main.mr`;
- impulse response mapping: `es/sound-library/impulse_responses.mr`;
- impulse response asset: `es/sound-library/new/minimal_muffling_02.wav`.

Principal verified parameters include:

| Parameter | Value in pinned script |
|---|---|
| Layout | 4 cylinders, two opposed banks at +90/-90 degrees |
| Bore | 99.5 mm |
| Stroke | 79 mm |
| Connecting rod length | 5.142 in |
| Starter torque/speed | 70 lb-ft / 500 RPM |
| Engine redline | 6,500 RPM |
| Ignition rev limiter | 6,800 RPM, 0.16 s limiter duration |
| Throttle | direct linkage, gamma 2.0 |
| Simulation frequency | 20,000 Hz |
| Exhaust systems | one |
| Audio tuning | high-frequency gain 0.01, noise 1.0, jitter 0.5 |
| Vehicle | 2,700 lb, drag 0.3, diff 3.9, tire radius 10 in |
| Transmission | six ratios: 3.636, 2.375, 1.761, 1.346, 0.971, 0.756 |

This choice is not an arbitrary declaration of the final Nextcar vehicle identity. It is the exact default exercised by the pinned engine-sim checkout and therefore minimizes configuration ambiguity for an integration proof.

### 8.2 Headless representation

NC-003B shall transcribe the fixture into a deterministic C++ builder that constructs the engine, required internal vehicle/transmission, cam profiles, timing curve, ignition wiring, intake/exhaust, and simulator objects directly.

Runtime `.mr` parsing is forbidden. A new general-purpose fixture data format is also out of scope because only one immutable fixture is required.

The impulse response shall be decoded offline by a deterministic tool and committed as a generated `int16_t` C++ array with sample count, source SHA-256, converter version, and generation command in the manifest. Runtime WAV file I/O is forbidden.

The pinned synthesizer's process-global `rand()` use shall be replaced inside the vendored boundary with an instance-owned `std::minstd_rand`, seeded with `0x4E433033` on each `Start`. The patch must be listed in the provenance manifest. No process-global `srand()` is permitted.

The fixture test shall verify identity and structural invariants, including name, cylinder/bank/crank/intake/exhaust counts, bore/stroke/rod dimensions, redline, simulation frequency, firing order/timing samples, vehicle/transmission values, impulse-response sample metadata, and successful deterministic lifecycle.

## 9. Telemetry schema

### 9.1 Machine report

NC-003D shall emit UTF-8 JSON with schema identifier:

```text
nextcar.engine_sim.benchmark.v1
```

One report represents one run. Aggregate JSON is mandatory; optional time-series CSV uses one header row and SI units in field names. Durations use seconds in JSON and may use microseconds in CSV only when the column name says `_us`.

### 9.2 Required fields

| Field | Type | Unit/meaning |
|---|---|---|
| `schema` | string | Exact schema identifier. |
| `run_id` | string | Unique run identifier. |
| `core.repository` | string | `Dziuras98/engine-sim`. |
| `core.pinned_commit` | string | Exact 40-hex pin. |
| `core.source_manifest_sha256` | string | Hash of canonical manifest bytes. |
| `fixture.id` | string | `subaru_ej25_atg_video_2_01`. |
| `started` / `shutdown_clean` | boolean | Lifecycle status. |
| `start_status` / `shutdown_status` | string | Stable status-code name. |
| `elapsed_wall_seconds` | number | Monotonic wall duration. |
| `simulation_seconds` | number | Accumulated simulated time. |
| `real_time_factor` | number | simulation/wall duration. |
| `simulation_frequency_hz` | integer | Configured physics frequency. |
| `simulation_iteration_us.mean/p95/max` | number | Per-step time. |
| `audio_production_us.mean/p95/max` | number | Per `PullPcm` call time. |
| `requested_frames` / `produced_frames` | integer | Mono PCM frames. |
| `sample_rate_hz` / `channel_count` | integer | Public PCM format. |
| `ring.capacity_frames` | integer | Runtime ring capacity. |
| `ring.fill_frames.min/mean/max` | number | Fill observations after operations. |
| `ring.underrun_events` / `ring.underrun_frames` | integer | Consumer deficits. |
| `ring.overrun_events` / `ring.overrun_frames` | integer | Rejected producer blocks/frames. |
| `rpm.current/mean/min/max` | number | RPM observations. |
| `controls.throttle_target_normalized` | number | Final target. |
| `controls.load_target_newton_metres` | number | Final target. |
| `pcm.peak_absolute` | number | Max absolute normalized sample. |
| `pcm.rms` | number | Root-mean-square normalized sample. |
| `pcm.clipped_samples` | integer | Samples with absolute value >= 1.0 after conversion. |
| `pcm.clipping_ratio` | number | clipped/produced scalar samples. |
| `errors` | array | Timestamped status code/message records. |

Percentiles use nearest-rank p95 over all recorded observations. Simulation iteration time is `Advance` call wall duration divided by the number of simulation steps reported for that call. Audio production time measures each `PullPcm` invocation. Fill is sampled after each producer write attempt and consumer read.

The optional CSV shall contain at least wall time, simulation time, RPM, throttle, load, requested/produced frames, ring fill, cumulative underrun/overrun counters, PCM peak/RMS, simulation duration, and audio-production duration.

## 10. Non-overlapping ownership

### NC-003B — Core

Owns writes to:

- `Plugins/NextcarEngineSim/Source/ThirdParty/EngineSim/**`;
- `Plugins/NextcarEngineSim/Source/NextcarEngineSimCore/**`;
- `Tools/EngineSimVendor/**`;
- `Tests/EngineSimCore/**`;
- `ThirdPartyNotices/**`.

Responsibilities: pin and minimal sources, transitive dependency proof, portable Core API, deterministic fixture, offline impulse generation, provenance/licenses, native audio adaptation, lifecycle, and standalone Core tests.

Must not edit Runtime, benchmark, gameplay, `.uproject`, existing workflows, plugin manifest, or manager history.

### NC-003C — Runtime Audio

Owns writes to:

- `Plugins/NextcarEngineSim/Source/NextcarEngineSimRuntime/**`.

Responsibilities: fake Core/test double, dedicated worker, latest-value controls, SPSC ring, lifecycle, `USynthComponent`/`ISoundGenerator`, underrun/overrun behavior, and Runtime Unreal Automation Tests.

Must compile and test against the frozen interface without requiring the real Core implementation. Must not edit vendored/Core files, benchmark, gameplay, existing workflows, plugin manifest, or manager history.

### NC-003D — Measurement

Owns writes to:

- `Tools/EngineSimBenchmark/**`;
- `.github/workflows/engine-sim-benchmark.yml`;
- benchmark-owned schemas/sample reports under `Tools/EngineSimBenchmark/**`.

Responsibilities: standalone benchmark, smoke/stress/soak harness, JSON/CSV schema, percentile/statistical aggregation, ring analysis, CI command, artifacts, and logs.

It consumes the frozen Core API. Runtime ring analysis uses a benchmark-owned portable model or Runtime test seam agreed in the manager scaffold; NC-003D does not edit Runtime source.

### Manager/integration reservation

Reserved from all three parallel workstreams:

- `Nextcar.uproject`;
- `Source/Nextcar/**`;
- `Plugins/NextcarEngineSim/NextcarEngineSim.uplugin`;
- existing `.github/workflows/*.yml` files;
- `docs/manager-history.md`;
- changes to this contract;
- connection of real Core to Runtime;
- final integration order and merges.

The manager shall create the shared plugin scaffold before parallel implementation or integrate it sequentially. This prevents conflicts in the plugin manifest and project registration.

## 11. Test matrix and gates

| Test | Owner/stage | Required assertion |
|---|---|---|
| Repository validator | Every PR | `python scripts/validate_repository.py` passes. |
| Vendor verifier | NC-003B and integration | Pin, manifest paths, hashes, and notices match. |
| Standalone Core compilation/tests | NC-003B | GCC/Clang where supported and Windows toolchain compile; tests pass. |
| Deterministic fixture | NC-003B | Structural invariants and repeatable lifecycle/control schedule. |
| Initialization/shutdown/restart | B and C | Clean success, failure cleanup, idempotent stop, no surviving thread. |
| Throttle/load response | B | Measurable RPM difference under a fixed timed schedule. |
| Non-empty PCM | B | After startup zero-prefix drain, produced frames > 0, RMS > 0, finite samples. |
| Ring unit tests | C | Wraparound, exact capacity, ordering, partial consume, all-or-nothing producer write. |
| Underrun/overrun tests | C | Exact zero-fill/rejection and counters; no block/allocation. |
| Fake-Core Runtime tests | C | Start/prefill/generate/stop/restart/failure behavior without real engine-sim. |
| Benchmark smoke | D | Complete valid JSON and deterministic exit status. |
| Soak | D/integration | Sustained schedule, complete artifacts, no under/over/crash/deadlock/leak symptom. |
| `NextcarEditor Win64 Development` | Integration | Successful UBT build. |
| All `Nextcar.*` Automation Tests | Integration | Complete successful report. |
| Manual audio smoke | Integration | Audible continuity and RPM/control response recorded. |

### 11.1 Per-PR short test

The per-PR benchmark smoke shall run for 60 wall seconds after successful startup with a fixed schedule:

- 0-10 s: throttle 0.15, load 0;
- 10-25 s: throttle 0.50, load 0;
- 25-40 s: throttle 0.75, load 300 N·m;
- 40-55 s: throttle 0.35, load 600 N·m;
- 55-60 s: throttle 0.10, load 0.

It requires valid JSON, clean start/stop, finite RPM, measurable RPM response, non-empty finite PCM, and no process failure. It is a functional smoke, not the final real-time scheduling proof.

### 11.2 Windows self-hosted soak

The sustained test runs for 30 wall minutes on `[self-hosted, Windows, X64, unreal-5.8]`, repeatedly applying the same schedule while driving producer and consumer according to recorded Unreal callback sizes. It uploads JSON, optional CSV, stdout/stderr, and build/test logs.

Non-negotiable first-spike gates:

- zero underrun events and frames;
- zero overrun events and frames;
- no crash, deadlock, or test-detectable leak;
- controlled successful start and shutdown;
- continuous non-empty finite PCM after priming;
- measurable RPM response to controls;
- every required telemetry field present and schema-valid;
- no gameplay dependency on SDL, UI, Discord, or scripting.

A large buffer does not excuse a real-time factor below sustainable production. The report must expose the condition for manager review.

### 11.3 Manual audio smoke

Using the integrated build and fixed fixture:

1. Start from a clean editor launch.
2. Start, stop, and restart the source.
3. Hold low, medium, and high throttle.
4. Apply and remove load at steady throttle.
5. Pause/resume and stop Play-in-Editor.
6. Listen for gaps, repeated stale blocks, clicks at lifecycle boundaries, runaway level, or silence.
7. Confirm audible pitch/RPM response and preserve the matching telemetry report.

The manual test does not replace automated counters or the soak.

No hard CPU or final latency budget is set in NC-003A. NC-003D reports measurements; the manager sets budgets from the actual Windows runner evidence.

## 12. Required integration order

1. Merge NC-003A.
2. Manager adds/reserves the shared plugin scaffold without implementing integration behavior.
3. Run NC-003B, NC-003C, and NC-003D in parallel against this contract.
4. Integrate NC-003B Core after vendor verification and Core tests pass.
5. Integrate NC-003D benchmark and obtain the first real Core measurement report.
6. Integrate NC-003C Runtime against its fake Core after Runtime tests pass.
7. Manager connects the real Core factory to Runtime without changing public semantics.
8. Run complete repository validation, vendor verification, standalone tests, benchmark smoke, `NextcarEditor Win64 Development`, all `Nextcar.*` tests, and the 30-minute sustained test.
9. Run and record the manual audio smoke.
10. Only after all gates pass, begin a separate engine/clutch/gearbox/driveline model and gameplay coupling task.

A failed gate blocks the next stage. A source pin, fixture, public unit, thread model, Unreal generator mechanism, or ownership change requires explicit manager review and a contract revision.

## 13. Licensing and distribution

NC-003B shall preserve engine-sim's complete MIT notice in `ThirdPartyNotices/engine-sim.txt`, retain applicable source notices, and include the notice in distributed builds containing substantial engine-sim portions.

For each copied dependency or asset, provenance must record repository, exact commit, source path, hash, copyright, license/SPDX status, notice obligations, and redistribution analysis. This includes, as applicable:

- `simple-2d-constraint-solver`;
- selected `delta-basic`/delta-studio sources;
- `csv-io` if proven necessary after source minimization;
- the Subaru script-derived fixture;
- `minimal_muffling_02.wav` and its generated array.

The main repository's MIT finding does not automatically license submodules or assets. Piranha, Discord, direct-to-video, upstream GoogleTest fetching, and original rendering/application sources shall not be included merely because they exist upstream.

The packaged game shall not depend on a developer checkout, external CMake, runtime script files, runtime external WAV paths, untracked prebuilt binaries, or network access. A later prebuilt strategy requires its own reproducible toolchain/CRT/configuration matrix, symbols, checksums, and updated notices.

## 14. Frozen decision register and measurement risks

| Decision | Selected | Rejected | Evidence/consequence | Owner |
|---|---|---|---|---|
| Import | Minimal vendored source snapshot with manifest/hashes. | Submodule, build-time CMake, prebuilt library. | Repository-local UBT/offline packaging, source debugging, explicit updates. | B |
| Core boundary | Portable `IEngineSimCore`, no Unreal/upstream public types. | Exposed `Simulator` or UObject Core. | Enables independent Core, fake Runtime, and benchmark. | B/manager |
| Fixture | Default pinned Subaru EJ25 as C++ builder. | Runtime Piranha, arbitrary engine, new generic data format. | Exact pinned default, deterministic and script-free runtime. | B |
| Impulse response | Offline generated `int16_t` array with provenance. | Runtime WAV I/O or omitted asset. | Deterministic package, no runtime file operation. | B |
| Randomness | Per-instance `minstd_rand`, fixed seed. | Global `rand()`/`srand()`. | Repeatability and instance isolation. | B |
| Public PCM | float32 mono, 44.1 kHz. | Public int16, invented configurable native rate, stereo duplication. | Native verified format plus Unreal float generator. | B/C |
| Unreal source | `CreateSoundGenerator` + `ISoundGenerator`. | Direct deprecated-path callback, original app audio. | Official UE 5.8 API/thread model. | C |
| Physics owner | Dedicated Runtime worker. | Game/audio thread physics or shared mutable Core. | Real-time-safe callback and single simulator owner. | C |
| Native synth | Private Core implementation thread for spike. | Runtime exposure or audio-thread physics. | Matches pinned synthesizer and bounded lifecycle. | B |
| Controls | Latest coherent throttle/load mailbox. | Unbounded queue/direct game-thread calls. | Continuous targets may coalesce safely. | C |
| Load | Absolute opposing dynamometer torque in N·m. | Fake gearbox coupling or undocumented normalized load. | Isolated verified load mechanism. | B |
| Runtime buffer | Preallocated bounded SPSC, initial 8,192 frames. | Mutex/unbounded/overwrite-oldest. | Deterministic memory and audio callback safety. | C/D |
| Underrun | Zero-fill and count; never block. | Wait, stale repetition, hidden deficit. | Callback continuity; soak still fails. | C/D |
| Overrun | Reject newest whole block and count. | Overwrite, resize, block. | Preserve unread ordering; soak still fails. | C/D |
| Report | Versioned aggregate JSON; optional CSV. | Log-only or Unreal-only stats. | Machine-verifiable CI and later HUD reuse. | D |
| Gameplay | Reserved for later manager task. | Editing arcade movement in B/C/D. | Current gameplay has no permanent powertrain. | Manager |
| CPU/latency | Measure first. | Invented hard threshold. | No baseline exists on the target runner. | D/manager |

The only unresolved risks allowed after this contract are measurement or portability risks:

- mean/p95/max simulation cost at 20,000 Hz on the Windows runner;
- whether another simulation frequency gives a better sustainable quality/cost point;
- optimal ring capacity, prefill, watermarks, and producer block after callback/scheduling data;
- startup duration needed to drain the native zero prefix and prefill Runtime;
- actual Unreal callback frame sizes and target device sample rate;
- audible quality of Unreal's source-rate conversion when the mixer device is not 44.1 kHz;
- long-run numerical and native synthesizer stability;
- UBT/MSVC portability defects in the minimal upstream source set;
- service-runner background-load sensitivity;
- final CPU and latency budgets derived from NC-003D reports.

These risks do not authorize an implementation agent to change the pinning model, fixture identity or representation, public API/units, thread ownership, Unreal generator mechanism, ring ownership, or workstream boundaries.

NC-003B, NC-003C, and NC-003D may begin after this contract and the manager-owned plugin scaffold are merged. A workstream is complete only when its owned tests, provenance, evidence, and handoff satisfy this contract. The integrated spike must pass every non-negotiable gate before any powertrain coupling begins.
