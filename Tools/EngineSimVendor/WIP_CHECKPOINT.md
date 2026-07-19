# NC-003B Phase 0 WIP checkpoint

Status: Phase 0 WIP — not submitted for approval
Phase 1: not started
Current blocker: exact binary transfer of minimal_muffling_02.wav

This checkpoint preserves the locally developed Phase 0 architecture and calibration work. It is not a Phase 0 submission and it does not contain a trusted copy of the required WAV.

## Pinned inputs

| Input | Repository | Commit | Required path/blob |
|---|---|---|---|
| engine-sim | `Dziuras98/engine-sim` | `85f7c3b959a908ed5232ede4f1a4ac7eafe6b630` | `es/sound-library/new/minimal_muffling_02.wav`, blob `6d3f8688e170cb6e5f4bfec42f580f3900514d72` |
| solver | `ange-yaghi/simple-2d-constraint-solver` | `e009f4ff1c9c4c5874e865e893cdb62e208fb2b3` | minimal NSV/Gauss-Seidel closure |

## Current minimal closure

| Layer | Included units | Reason |
|---|---|---|
| Foundation | `Part`, constants, units, utilities, `Function`, Gaussian filter | data model, interpolation and fixture functions |
| Gas/combustion | `GasSystem`, `Fuel`, `Intake`, `ExhaustSystem`, `CombustionChamber` | cylinder pressure, flow and combustion |
| Mechanics | `Crankshaft`, `ConnectingRod`, `Piston`, `CylinderBank`, `CylinderHead`, `Camshaft`, `StandardValvetrain` | EJ25 kinematics and valve timing |
| Ignition/start | `IgnitionModule`, `StarterMotor`, direct throttle linkage | deterministic cold start |
| Audio | convolution, derivative, low-pass, Butterworth, jitter, leveler, delay, ring buffer, synchronous `Synthesizer` | deterministic single-threaded PCM path |
| Simulation | headless `Engine`, `Simulator`, `PistonEngineSimulator` | no vehicle, transmission, dyno or gameplay coupling |
| Solver | optimized NSV rigid-body system, Gauss-Seidel SLE, fixed-position, line, link, clutch and rotation-friction constraints | exact mechanical path used by `PistonEngineSimulator` |
| Fixture | `SubaruEJ25AtgVideo2Fixture` | deterministic transcription of the pinned EJ25 fixture |

Explicitly excluded: renderer/UI, scripting runtime, vehicle, transmission, dynamometer, generic solver path, RK4/Euler alternatives, gravity/spring/demo code, Runtime Audio, benchmark and gameplay.

## Current patch inventory

| Patch | State | Purpose |
|---|---|---|
| `matrix.cpp` adds `<cstring>` | implemented locally before checkpoint | portable `memset` declaration |
| `sparse_matrix.h` includes `matrix.h` | implemented locally before checkpoint | complete type required by templates |
| `constraint.cpp` uses `sizeof(m_bodies)` | implemented locally before checkpoint | correct pointer-array zeroing |
| optimized NSV reset deletes/nulls SLE solver | implemented locally before checkpoint | ownership cleanup |
| constants/units use `inline constexpr` | implemented locally before checkpoint | C++17 ODR portability |
| `camshaft.cpp` adds `<cstring>` | implemented locally before checkpoint | portable `memset` declaration |
| connecting-rod Windows backslash include removed | implemented locally before checkpoint | cross-platform include path |
| explicit ring-buffer occupancy | implemented locally before checkpoint | distinguish full from empty |
| synchronous synthesizer | implemented locally before checkpoint | no native renderer thread or condition-variable worker |
| per-instance deterministic RNG | implemented locally before checkpoint | remove global `rand()` dependence |
| function-static mutable audio state removed | implemented locally before checkpoint | instance isolation and repeatability |
| simulator drivetrain/dyno removal | implemented locally before checkpoint | headless Phase 0 scope |
| exact harmonic-cam-lobe generation | implemented locally before checkpoint | match native script node algorithm |

## Local test evidence retained from the WIP session

| Command/test | Exit code | Result/limitation |
|---|---:|---|
| Toolchain discovery: CMake, Ninja, GCC, Clang, Git, Python | 0 | CMake 3.31.6; Ninja 1.12.1; GCC 14.2.0; Clang 17.0.0; Git 2.47.3; Python 3.13.5 |
| GCC solver smoke build and execution | 0 | printed `solver-smoke-ok`; upstream unused-parameter/sign warnings remained |
| GCC compilation of the current engine/fixture closure | 0 | compiled after portability/ODR patches |
| Preliminary synchronous cold-start candidate runs using a one-sample identity IR | 0 each | PCM was produced and engine remained running after exact cam-lobe fix; not valid final calibration evidence |

Preliminary candidate observations used an identity impulse response, not the required `minimal_muffling_02.wav`. Startup throttle near `0` and starter disengagement near `700 RPM` remain hypotheses only.

## Not yet executed

- trusted checkout and local Git-blob verification of the required WAV;
- WAV RIFF/PCM metadata and decoded-PCM SHA-256 measurements;
- deterministic generated impulse-response header and independent PCM comparison;
- complete `SOURCE_MANIFEST.json` and vendor verifier;
- final trace/sweep, ten identical trials, timeout negative case and stall-after-disengagement negative case;
- cold-start JSON/header consistency;
- final GCC and Clang matrices;
- sanitizers;
- UBT/MSVC, `NextcarEditor Win64 Development`, and all `Nextcar.*` Unreal Automation Tests.

No test above is considered passed merely by static inspection.
