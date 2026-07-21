# Subaru EJ25 cold-start calibration

> Status: Phase 0 WIP — cold-start calibration manager-accepted and unchanged; final closure Windows exact-head validation pending. Phase 1 not started.

## Validated source

- Candidate source SHA: `ddfe39c5179663c1636520a1fa7a92d8c4852f99`
- Workflow run ID: `29814599329`
- Final run attempt: `2`
- Artifact ID: `8488882253`
- Artifact name: `NC-003B-cold-start-selected-profile-ddfe39c5179663c1636520a1fa7a92d8c4852f99`
- Artifact ZIP SHA-256: `f7b9004e24a81f1558c1df39a7d27dd31a9cc0a916a46f00e6f4b10881b1727b`
- Artifact file count: `41`
- Runner image: `windows-2022/20260714.244.1`
- Visual Studio: `17.14.37411.7`
- MSVC: `Microsoft (R) C/C++ Optimizing Compiler Version 19.44.35228 for x64`
- VCTools: `14.44.35207`
- Windows SDK: `10.0.26100.0`
- CMake: `cmake version 3.31.6`
- Python: `Python 3.12.10`

## Sweep

- Evaluated candidates: `1080`
- Discovery traces: `30`
- Startup throttle: `0.05, 0.10, 0.20, 0.35, 0.50`
- Starter disengagement RPM: `500, 650, 800, 950, 1100, 1300`
- Post-starter minimum RPM: `400, 500, 600, 700`
- Stability windows: `0.5, 1.0, 2.0 s`
- Maximum startup times: `2.0, 4.0, 8.0 s`

## Selected candidate

- Candidate ID: `thr-0p050-dis-0800-min-0600-win-2p000-max-8p000`
- Startup throttle: `0.05`
- Starter disengagement criterion: RPM >= `800.0`
- Post-starter minimum RPM: `600.0`
- Stability window: `2.0 s`
- Maximum startup simulation time: `8.0 s`
- Trials: `10`
- Successes: `10`
- Failures: `0`

## Selected margins

- Post-starter RPM margin: `139.20105083299995`
- Timeout margin: `4.058333333 s`
- Disengagement separation: `6.944407439999964 RPM`

## Distributions

- `disengagement_rpm`: min `806.94440744`, max `806.94440744`, mean `806.94440744`, median `806.94440744`, p95 `806.94440744`
- `disengagement_time_seconds`: min `1.941666667`, max `1.941666667`, mean `1.941666667`, median `1.941666667`, p95 `1.941666667`
- `mean_post_starter_rpm`: min `814.285833297`, max `814.285833297`, mean `814.285833297`, median `814.285833297`, p95 `814.285833297`
- `minimum_post_starter_rpm`: min `739.201050833`, max `739.201050833`, mean `739.201050833`, median `739.201050833`, p95 `739.201050833`
- `post_start_pcm_rms`: min `0.140563552`, max `0.140563552`, mean `0.140563552`, median `0.140563552`, p95 `0.140563552`
- `produced_native_frames`: min `78991.0`, max `78991.0`, mean `78991.0`, median `78991.0`, p95 `78991.0`
- `produced_pcm_frames`: min `174175.0`, max `174175.0`, mean `174175.0`, median `174175.0`, p95 `174175.0`

## Negative cases

- Timeout: `Timeout` — `Tests/EngineSimCore/Fixtures/subaru_ej25_cold_start_calibration/negative-timeout.json` — SHA-256 `2c4f7780706b78e674f7d121085ff31c2d4af91b963cc3da501af197dfc817ed`
- Stall after disengagement: `StallAfterDisengagement` — `Tests/EngineSimCore/Fixtures/subaru_ej25_cold_start_calibration/negative-stall.json` — SHA-256 `44d437f560a23e2435f0e4ecdf35b9be7d50bdd1e0d5c237c857ac5eb2ad043b`

## Positive traces

- Trial `0`: `Tests/EngineSimCore/Fixtures/subaru_ej25_cold_start_calibration/positive-traces/trial-00.json` — SHA-256 `8eb297a99da10a4e97aebba37e995cd0813d20b7873ad9b7648cc58c6c5c38d0`
- Trial `1`: `Tests/EngineSimCore/Fixtures/subaru_ej25_cold_start_calibration/positive-traces/trial-01.json` — SHA-256 `3e4a8d5f9d1d8d5eb6d828dbd598385afd96b3439ef879cbe63909c7775b8263`
- Trial `2`: `Tests/EngineSimCore/Fixtures/subaru_ej25_cold_start_calibration/positive-traces/trial-02.json` — SHA-256 `e19a045374125f6ba0015fded0849518b1d07e6e9786ecbdceebd5266b128478`
- Trial `3`: `Tests/EngineSimCore/Fixtures/subaru_ej25_cold_start_calibration/positive-traces/trial-03.json` — SHA-256 `bc97a4b197465a29a384cba4bde1e28c5a739ee7bfb84296275d9ff9a121127d`
- Trial `4`: `Tests/EngineSimCore/Fixtures/subaru_ej25_cold_start_calibration/positive-traces/trial-04.json` — SHA-256 `c69336da2b20b0856537eba81abaac8d637c6e0bec8df105113b0ec71ba78841`
- Trial `5`: `Tests/EngineSimCore/Fixtures/subaru_ej25_cold_start_calibration/positive-traces/trial-05.json` — SHA-256 `5671b43de33bba3c35f88fbd7d6e88707c1a4ad6b328c1eb945675930488d50c`
- Trial `6`: `Tests/EngineSimCore/Fixtures/subaru_ej25_cold_start_calibration/positive-traces/trial-06.json` — SHA-256 `af1efba664b9b57d6f2bb05fd040f691ca7ab8522c301bed439fe7d2470a31e5`
- Trial `7`: `Tests/EngineSimCore/Fixtures/subaru_ej25_cold_start_calibration/positive-traces/trial-07.json` — SHA-256 `d60b3d3cab44e02057863d6f6288ae44eb8b2381a2463b595a30daae2d7512bb`
- Trial `8`: `Tests/EngineSimCore/Fixtures/subaru_ej25_cold_start_calibration/positive-traces/trial-08.json` — SHA-256 `d7b9d47ca5bca6bb224506f0f4a388c2d81fd67da3b9a804a5c0344ef84a3867`
- Trial `9`: `Tests/EngineSimCore/Fixtures/subaru_ej25_cold_start_calibration/positive-traces/trial-09.json` — SHA-256 `d3e56038fa6f22b9adf7ad18a480de8cb072806c0f471bf631c54d777ecd0799`

## Generated inputs

- Profile JSON SHA-256: `2e8a6795c85e648606e174a00cc96230363b6bc60a78cd62374490377414bb1d`
- Profile payload SHA-256: `e94ea6572b51be523ed7b33a5786865303a18e14ef3ee0fb14a3edeb48d47d49`
- Generated header SHA-256: `8d44a17e4491dfdb1500ff8498c739c143fffa84510df2f31bfdf9a0e3797d5f`
- Generator SHA-256: `208790f300970ef004fbe57816231685a0a4d2013d48339ffc6467d53d14cb4f`
- Verifier SHA-256: `63f6bfe15692228bd1dd66661c2bf24f3a45d543acd25be2f67fbe86369ef008`
- Generator command: `python Tools/EngineSimVendor/generate_cold_start_profile_header.py --profile Tests/EngineSimCore/Fixtures/subaru_ej25_cold_start_profile.json --output Plugins/NextcarEngineSim/Source/NextcarEngineSimCore/Private/Generated/SubaruEJ25ColdStartProfile.generated.h`

## Windows validation

- Python tests: `PASS`
- MSVC x64 Release: `PASS`
- MSVC x64 Debug `/RTC1`: `PASS`
- MSVC x64 AddressSanitizer: `PASS`
- Selected-profile trials: `10/10 PASS`
- Timeout case: `PASS`
- Stall case: `PASS`
- Clean reproducibility: `PASS`
- Project warning count: `0`

The fixture transcription, mechanical values, engine-sim pin, solver pin, exact WAV, and generated impulse response were not changed.

## Publication validation and closure linkage

- Publication validation run: `29818165243`
- Publication validation artifact: `8490296560`
- Correct publication artifact SHA-256: `28a63d12a3b7565957c57cb758aa35ca0bdfac914c127525c30bb72816a90ec5`
- Selected-profile artifact SHA-256 remains `f7b9004e24a81f1558c1df39a7d27dd31a9cc0a916a46f00e6f4b10881b1727b`.

The minimal fixture-source snapshot and removal of temporary SourceInputs do not change the accepted profile ID, throttle, disengagement threshold, post-starter minimum, stability window, maximum simulation time, trace hashes, profile JSON semantics or generated-header values. Final Phase 0 closure validation must run on Windows against the published closure candidate before manager submission.

## Final Phase 0 closure validation evidence

Status at this commit:

```text
Phase 0 WIP — final closure candidate Windows-validated and evidence recorded;
exact evidence-head publication gate required before manager submission.

Phase 1 not started.
```

Target publication requires a separate full Windows exact-head run against this evidence commit.

Validated Checkpoint 2 and publication boundary:

```text
checkpoint SHA: 7cc91bf136554ca6a40bbbeb90a557ec4bef078c
checkpoint tree: b616b2ae12a51b4df83ba05858ce96cbbdce75e6
target SHA: 9bb6464123228f1d9638d467e7160a7cb2aa98c8
ahead: 2
behind: 0
workflow-fix SHA: a25c6af112e667c47ee77aaba33c5cd9d992ea6a
```

Closure-candidate Windows run and artifact:

```text
run ID: 29861088654
attempt: 1
artifact ID: 8507410452
artifact name: NC-003B-Phase0-final-closure-7cc91bf136554ca6a40bbbeb90a557ec4bef078c
GitHub digest: sha256:072228b427afa63f191c547fbb1a7aa8ad1b754fe2c1b31ceaf881159264f989
independent ZIP SHA-256: 072228b427afa63f191c547fbb1a7aa8ad1b754fe2c1b31ceaf881159264f989
file count: 64
hash-manifest entries: 63
hash-manifest: 63/63 PASS
```

The closure artifact identity is defined by GitHub artifact metadata: ID 8507410452, name `NC-003B-Phase0-final-closure-7cc91bf136554ca6a40bbbeb90a557ec4bef078c`, and ZIP SHA-256 `072228b427afa63f191c547fbb1a7aa8ad1b754fe2c1b31ceaf881159264f989`.

The artifact's inherited `toolchain-versions.json` `artifact_name` field uses a cold-start validator label and is non-authoritative.

Runner and full toolchain:

```text
runner: GitHub-hosted windows-2022
runner image: windows-2022/20260714.244.1
runner version: 2.336.0
operating system: Windows Server 2022 x64
Visual Studio: 17.14.37411.7
MSVC: Microsoft (R) C/C++ Optimizing Compiler Version 19.44.35228 for x64
VC tools: 14.44.35207
Windows SDK: 10.0.26100.0
CMake: cmake version 3.31.6
Python: Python 3.12.10
Git: git version 2.55.0.windows.2
```

Boundary and dependency proofs:

```text
autocrlf-proof: PASS
boundary-proof: PASS
SourceInputs absence: PASS
SourceInputs directory absent: PASS
tracked SourceInputs count: 0
network scan: PASS
network scan hit count: 0
runtime WAV scan: PASS
runtime WAV scan hit count: 0
post-matrix cleanup: PASS
git status clean after matrix: PASS
temporary workflow absent from candidate tree: PASS
```

Validation matrix:

```text
repository validation: PASS
git diff --check: PASS
snapshot verifier: PASS
snapshot tests: 7/7 PASS
fixture mutation tests: 24/24 PASS
fixture parity: 118/118 PASS
IR mutation tests: 11/11 PASS
IR verifier: PASS
IR generator determinism: PASS
cold-start mutation tests: 15/15 PASS
cold-start verifier: PASS
cold-start generator determinism: PASS
profile/header consistency: PASS
MSVC x64 Release: PASS
MSVC x64 Debug /RTC1: PASS
MSVC x64 AddressSanitizer: PASS
CTest: PASS
direct runtime: PASS
selected trials: 10/10 PASS
timeout: PASS
stall: PASS
project warning count: 0
clean reproducibility: PASS
post-matrix cleanup: PASS
```
